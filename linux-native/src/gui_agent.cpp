/*
 * Copyright (c) 2025 Bytedance, Inc. and its affiliates.
 * SPDX-License-Identifier: Apache-2.0
 */
#include "gui_agent.h"
#include <chrono>
#include <thread>
#include <condition_variable>

namespace uitars {

GUIAgent::GUIAgent(const Config& config) 
    : config_(config) {
    logger_ = Logger::getInstance();
    if (!logger_) {
        logger_ = std::make_shared<Logger>(config.logPath);
    }
    
    vlmClient_ = std::make_unique<VLMClient>(config, logger_);
    screenshot_ = std::make_unique<Screenshot>(logger_);
    inputController_ = std::make_unique<InputController>(logger_);
    actionParser_ = std::make_unique<ActionParser>(logger_);
    
    logger_->info("GUIAgent initialized");
}

GUIAgent::~GUIAgent() {
    stop();
}

bool GUIAgent::run(const std::string& instruction) {
    if (running_) {
        logger_->warn("Agent is already running");
        return false;
    }
    
    running_ = true;
    stopped_ = false;
    loopCount_ = 0;
    
    logger_->info("Starting agent with instruction: " + instruction);
    
    AgentData data;
    data.instruction = instruction;
    data.modelName = vlmClient_->getModelName();
    data.status = Status::RUNNING;
    data.logTime = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();
    
    // Add initial instruction to conversations
    Conversation initConv;
    initConv.from = "human";
    initConv.value = instruction;
    initConv.startTime = data.logTime;
    initConv.endTime = data.logTime;
    initConv.costTime = 0;
    data.conversations.push_back(initConv);
    
    notifyData(data);
    
    int snapshotErrCnt = 0;
    
    while (running_ && !stopped_) {
        // Check pause status
        if (paused_) {
            data.status = Status::PAUSE;
            notifyData(data);
            
            std::unique_lock<std::mutex> lock(pauseMutex_);
            pauseCV_.wait(lock, [this]{ return !paused_ || stopped_; });
            
            if (stopped_) break;
            
            data.status = Status::RUNNING;
            notifyData(data);
        }
        
        // Check loop count
        if (loopCount_ >= MAX_LOOP_COUNT) {
            logger_->error("Reached maximum loop count");
            data.status = Status::ERROR;
            notifyError({ErrorStatus::REACH_MAXLOOP_ERROR, "Reached maximum loop count", ""});
            break;
        }
        
        // Check snapshot error count
        if (snapshotErrCnt >= MAX_SNAPSHOT_ERR_CNT) {
            logger_->error("Too many screenshot failures");
            data.status = Status::ERROR;
            notifyError({ErrorStatus::SCREENSHOT_RETRY_ERROR, "Too many screenshot failures", ""});
            break;
        }
        
        loopCount_++;
        auto startTime = std::chrono::system_clock::now();
        
        logger_->info("Loop " + std::to_string(loopCount_));
        
        // Take screenshot
        ScreenshotOutput screenshotOutput;
        try {
            screenshotOutput = screenshot_->capture();
        } catch (const std::exception& e) {
            logger_->error("Screenshot failed: " + std::string(e.what()));
            snapshotErrCnt++;
            std::this_thread::sleep_for(std::chrono::seconds(1));
            loopCount_--;
            continue;
        }
        
        if (screenshotOutput.base64.empty()) {
            logger_->error("Screenshot returned empty");
            snapshotErrCnt++;
            std::this_thread::sleep_for(std::chrono::seconds(1));
            loopCount_--;
            continue;
        }
        
        auto endTime = std::chrono::system_clock::now();
        auto costTime = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();
        
        // Add screenshot to conversations
        Conversation screenshotConv;
        screenshotConv.from = "human";
        screenshotConv.value = "<image>";
        screenshotConv.screenshotBase64 = screenshotOutput.base64;
        screenshotConv.screenContext = {screenshotOutput.width, screenshotOutput.height, screenshotOutput.scaleFactor};
        screenshotConv.startTime = std::chrono::duration_cast<std::chrono::milliseconds>(startTime.time_since_epoch()).count();
        screenshotConv.endTime = std::chrono::duration_cast<std::chrono::milliseconds>(endTime.time_since_epoch()).count();
        screenshotConv.costTime = costTime;
        data.conversations.push_back(screenshotConv);
        
        notifyData(data);
        
        // Invoke VLM model
        VLMResponse vlmResponse;
        try {
            vlmResponse = vlmClient_->invoke(
                buildSystemPrompt(),
                data.conversations,
                screenshotOutput.width,
                screenshotOutput.height,
                screenshotOutput.scaleFactor
            );
        } catch (const std::exception& e) {
            logger_->error("VLM invoke failed: " + std::string(e.what()));
            data.status = Status::ERROR;
            notifyError({ErrorStatus::INVOKE_RETRY_ERROR, std::string("VLM invoke failed: ") + e.what(), ""});
            break;
        }
        
        if (vlmResponse.prediction.empty()) {
            logger_->warn("VLM returned empty prediction");
            continue;
        }
        
        logger_->info("VLM Response: " + vlmResponse.prediction);
        
        endTime = std::chrono::system_clock::now();
        costTime = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();
        
        // Add prediction to conversations
        Conversation predictionConv;
        predictionConv.from = "gpt";
        predictionConv.value = vlmResponse.prediction;
        predictionConv.screenContext = {screenshotOutput.width, screenshotOutput.height, screenshotOutput.scaleFactor};
        predictionConv.parsedPredictions = vlmResponse.parsedPredictions;
        predictionConv.startTime = screenshotConv.endTime;
        predictionConv.endTime = std::chrono::duration_cast<std::chrono::milliseconds>(endTime.time_since_epoch()).count();
        predictionConv.costTime = costTime - screenshotConv.costTime;
        data.conversations.push_back(predictionConv);
        
        notifyData(data);
        
        // Execute actions
        for (const auto& action : vlmResponse.parsedPredictions) {
            logger_->info("Executing action: " + action.actionTypeStr);
            
            if (action.actionType == ActionType::FINISHED) {
                data.status = Status::END;
                logger_->info("Task finished");
                break;
            }
            
            if (action.actionType == ActionType::CALL_USER) {
                data.status = Status::CALL_USER;
                logger_->info("Calling user for help");
                break;
            }
            
            if (!executeAction(action, screenshotOutput.width, screenshotOutput.height)) {
                logger_->error("Action execution failed");
            }
        }
        
        if (data.status == Status::END || data.status == Status::CALL_USER) {
            break;
        }
        
        // Sleep between loops
        if (config_.loopIntervalMs > 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(config_.loopIntervalMs));
        }
    }
    
    running_ = false;
    notifyData(data);
    
    logger_->info("Agent run completed with status: " + statusToString(data.status));
    
    return data.status == Status::END;
}

void GUIAgent::pause() {
    paused_ = true;
    logger_->info("Agent paused");
}

void GUIAgent::resume() {
    {
        std::lock_guard<std::mutex> lock(pauseMutex_);
        paused_ = false;
    }
    pauseCV_.notify_all();
    logger_->info("Agent resumed");
}

void GUIAgent::stop() {
    stopped_ = true;
    running_ = false;
    resume(); // Wake up if paused
    logger_->info("Agent stopped");
}

bool GUIAgent::isRunning() const {
    return running_;
}

bool GUIAgent::isPaused() const {
    return paused_;
}

void GUIAgent::setDataCallback(DataCallback callback) {
    dataCallback_ = std::move(callback);
}

void GUIAgent::setErrorCallback(ErrorCallback callback) {
    errorCallback_ = std::move(callback);
}

std::string GUIAgent::buildSystemPrompt() const {
    return R"(You are a GUI Agent that can control a computer through mouse and keyboard actions.
You can perform the following actions:
- click(start_box='[x1, y1, x2, y2]') - Click at the center of the bounding box
- left_double(start_box='[x1, y1, x2, y2]') - Double click at the center of the bounding box
- right_single(start_box='[x1, y1, x2, y2]') - Right click at the center of the bounding box
- drag(start_box='[x1, y1, x2, y2]', end_box='[x3, y3, x4, y4]') - Drag from start to end position
- hotkey(key='') - Press a keyboard shortcut (e.g., 'ctrl+c', 'alt+tab')
- type(content='') - Type text. Use '\n' at the end to submit.
- scroll(start_box='[x1, y1, x2, y2]', direction='down or up or right or left') - Scroll in direction
- wait() - Wait for 5 seconds and take a screenshot
- finished() - Mark the task as complete
- call_user() - Request user help when the task cannot be completed

Analyze the screenshot and determine the next action to accomplish the user's goal.
Provide your response in the format:
Thought: [Your reasoning about what to do next]
Action: [The action to take])";
}

bool GUIAgent::executeAction(const ParsedAction& action, int screenWidth, int screenHeight) {
    try {
        switch (action.actionType) {
            case ActionType::CLICK: {
                auto it = action.actionInputs.find("start_box");
                if (it != action.actionInputs.end()) {
                    Point p = actionParser_->parseBoxToScreenCoords(it->second, screenWidth, screenHeight);
                    inputController_->click(p.x, p.y);
                }
                break;
            }
            case ActionType::LEFT_DOUBLE: {
                auto it = action.actionInputs.find("start_box");
                if (it != action.actionInputs.end()) {
                    Point p = actionParser_->parseBoxToScreenCoords(it->second, screenWidth, screenHeight);
                    inputController_->doubleClick(p.x, p.y);
                }
                break;
            }
            case ActionType::RIGHT_SINGLE: {
                auto it = action.actionInputs.find("start_box");
                if (it != action.actionInputs.end()) {
                    Point p = actionParser_->parseBoxToScreenCoords(it->second, screenWidth, screenHeight);
                    inputController_->rightClick(p.x, p.y);
                }
                break;
            }
            case ActionType::DRAG: {
                auto startIt = action.actionInputs.find("start_box");
                auto endIt = action.actionInputs.find("end_box");
                if (startIt != action.actionInputs.end() && endIt != action.actionInputs.end()) {
                    Point start = actionParser_->parseBoxToScreenCoords(startIt->second, screenWidth, screenHeight);
                    Point end = actionParser_->parseBoxToScreenCoords(endIt->second, screenWidth, screenHeight);
                    inputController_->drag(start.x, start.y, end.x, end.y);
                }
                break;
            }
            case ActionType::HOTKEY: {
                auto it = action.actionInputs.find("key");
                if (it != action.actionInputs.end()) {
                    inputController_->hotkey(it->second);
                }
                break;
            }
            case ActionType::TYPE: {
                auto it = action.actionInputs.find("content");
                if (it != action.actionInputs.end()) {
                    inputController_->type(it->second);
                }
                break;
            }
            case ActionType::SCROLL: {
                std::string direction = "down";
                int x = screenWidth / 2;
                int y = screenHeight / 2;
                
                auto dirIt = action.actionInputs.find("direction");
                if (dirIt != action.actionInputs.end()) {
                    direction = dirIt->second;
                }
                
                auto boxIt = action.actionInputs.find("start_box");
                if (boxIt != action.actionInputs.end()) {
                    Point p = actionParser_->parseBoxToScreenCoords(boxIt->second, screenWidth, screenHeight);
                    x = p.x;
                    y = p.y;
                }
                
                inputController_->scroll(x, y, direction);
                break;
            }
            case ActionType::WAIT: {
                inputController_->wait(5000);
                break;
            }
            default:
                logger_->warn("Unknown action type: " + action.actionTypeStr);
                return false;
        }
        return true;
    } catch (const std::exception& e) {
        logger_->error("Action execution failed: " + std::string(e.what()));
        return false;
    }
}

void GUIAgent::notifyData(const AgentData& data) {
    if (dataCallback_) {
        dataCallback_(data);
    }
}

void GUIAgent::notifyError(const AgentError& error) {
    if (errorCallback_) {
        errorCallback_(error);
    }
}

} // namespace uitars
