/*
 * Copyright (c) 2025 Bytedance, Inc. and its affiliates.
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef UI_TARS_TYPES_H
#define UI_TARS_TYPES_H

#include <string>
#include <vector>
#include <map>
#include <cstdint>

namespace uitars {

// Status enumeration
enum class Status {
    INIT,
    RUNNING,
    PAUSE,
    END,
    ERROR,
    USER_STOPPED,
    CALL_USER
};

// Error status enumeration
enum class ErrorStatus {
    UNKNOWN_ERROR,
    SCREENSHOT_RETRY_ERROR,
    INVOKE_RETRY_ERROR,
    EXECUTE_RETRY_ERROR,
    REACH_MAXLOOP_ERROR,
    ENVIRONMENT_ERROR,
    MODEL_SERVICE_ERROR
};

// Action types
enum class ActionType {
    CLICK,
    LEFT_DOUBLE,
    RIGHT_SINGLE,
    DRAG,
    HOTKEY,
    TYPE,
    SCROLL,
    WAIT,
    FINISHED,
    CALL_USER,
    UNKNOWN
};

// Screenshot output structure
struct ScreenshotOutput {
    std::vector<uint8_t> imageData;
    std::string base64;
    int width;
    int height;
    double scaleFactor;
};

// Parsed action structure
struct ParsedAction {
    ActionType actionType;
    std::string actionTypeStr;
    std::map<std::string, std::string> actionInputs;
    std::string reflection;
    std::string thought;
};

// Screen context
struct ScreenContext {
    int width;
    int height;
    double scaleFactor;
};

// Conversation entry
struct Conversation {
    std::string from;  // "human" or "gpt"
    std::string value;
    std::string screenshotBase64;
    ScreenContext screenContext;
    std::vector<ParsedAction> parsedPredictions;
    int64_t startTime;
    int64_t endTime;
    int64_t costTime;
};

// Agent data structure
struct AgentData {
    std::string instruction;
    std::string modelName;
    Status status;
    std::vector<Conversation> conversations;
    int64_t logTime;
};

// Agent error structure
struct AgentError {
    ErrorStatus type;
    std::string message;
    std::string stack;
};

// Point structure for coordinates
struct Point {
    int x;
    int y;
};

// Box structure for regions
struct Box {
    int x1;
    int y1;
    int x2;
    int y2;
    
    Point center() const {
        return {(x1 + x2) / 2, (y1 + y2) / 2};
    }
};

// VLM response structure
struct VLMResponse {
    std::string prediction;
    std::vector<ParsedAction> parsedPredictions;
    int costTokens;
    int64_t costTime;
    std::string responseId;
};

// Convert ActionType to string
inline std::string actionTypeToString(ActionType type) {
    switch (type) {
        case ActionType::CLICK: return "click";
        case ActionType::LEFT_DOUBLE: return "left_double";
        case ActionType::RIGHT_SINGLE: return "right_single";
        case ActionType::DRAG: return "drag";
        case ActionType::HOTKEY: return "hotkey";
        case ActionType::TYPE: return "type";
        case ActionType::SCROLL: return "scroll";
        case ActionType::WAIT: return "wait";
        case ActionType::FINISHED: return "finished";
        case ActionType::CALL_USER: return "call_user";
        default: return "unknown";
    }
}

// Convert string to ActionType
inline ActionType stringToActionType(const std::string& str) {
    if (str == "click" || str == "left_click" || str == "left_single") return ActionType::CLICK;
    if (str == "left_double" || str == "double_click") return ActionType::LEFT_DOUBLE;
    if (str == "right_click" || str == "right_single") return ActionType::RIGHT_SINGLE;
    if (str == "drag" || str == "left_click_drag" || str == "select") return ActionType::DRAG;
    if (str == "hotkey") return ActionType::HOTKEY;
    if (str == "type") return ActionType::TYPE;
    if (str == "scroll") return ActionType::SCROLL;
    if (str == "wait") return ActionType::WAIT;
    if (str == "finished") return ActionType::FINISHED;
    if (str == "call_user") return ActionType::CALL_USER;
    return ActionType::UNKNOWN;
}

// Convert Status to string
inline std::string statusToString(Status status) {
    switch (status) {
        case Status::INIT: return "init";
        case Status::RUNNING: return "running";
        case Status::PAUSE: return "pause";
        case Status::END: return "end";
        case Status::ERROR: return "error";
        case Status::USER_STOPPED: return "user_stopped";
        case Status::CALL_USER: return "call_user";
        default: return "unknown";
    }
}

} // namespace uitars

#endif // UI_TARS_TYPES_H
