/*
 * Copyright (c) 2025 Bytedance, Inc. and its affiliates.
 * SPDX-License-Identifier: Apache-2.0
 */
#include "llama_inference.h"

#include <cstring>
#include <fstream>
#include <sstream>
#include <chrono>
#include <regex>
#include <cmath>
#include <stdexcept>

// Base64 decoding
static const std::string base64_chars = 
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

static std::vector<uint8_t> base64_decode(const std::string& encoded) {
    std::vector<uint8_t> decoded;
    std::vector<int> T(256, -1);
    for (int i = 0; i < 64; i++) {
        T[base64_chars[i]] = i;
    }
    
    int val = 0, valb = -8;
    for (unsigned char c : encoded) {
        if (T[c] == -1) break;
        val = (val << 6) + T[c];
        valb += 6;
        if (valb >= 0) {
            decoded.push_back((val >> valb) & 0xFF);
            valb -= 8;
        }
    }
    return decoded;
}

namespace uitars {

LlamaInference::LlamaInference(const LocalModelConfig& config, std::shared_ptr<Logger> logger)
    : config_(config), logger_(logger) {
    // llama.cpp backend will be initialized when loadModel is called
}

LlamaInference::~LlamaInference() {
    unloadModel();
}

bool LlamaInference::loadModel() {
    if (modelLoaded_) {
        logger_->warn("Model already loaded");
        return true;
    }

    logger_->info("Loading model from: " + config_.modelPath);
    
    // Check if model file exists
    std::ifstream modelFile(config_.modelPath);
    if (!modelFile.good()) {
        logger_->error("Model file not found: " + config_.modelPath);
        return false;
    }
    modelFile.close();

#ifdef HAS_LLAMA
    // TODO: Initialize llama.cpp backend when ENABLE_LLAMA is set
    // The build system will fetch and link llama.cpp
    logger_->info("llama.cpp backend available");
#else
    logger_->warn("Built without llama.cpp support. Local inference will use stub responses.");
    logger_->warn("Rebuild with: cmake -DENABLE_LLAMA=ON for real local inference.");
#endif
    
    modelLoaded_ = true;
    logger_->info("Model registered: " + getModelName());
    
    return true;
}

void LlamaInference::unloadModel() {
    // Clean up llama.cpp resources when implemented
    modelLoaded_ = false;
    logger_->info("Model unloaded");
}

bool LlamaInference::isLoaded() const {
    return modelLoaded_;
}

std::string LlamaInference::getModelName() const {
    // Extract model name from path
    std::string path = config_.modelPath;
    size_t lastSlash = path.find_last_of("/\\");
    if (lastSlash != std::string::npos) {
        path = path.substr(lastSlash + 1);
    }
    // Remove .gguf extension
    size_t dotPos = path.rfind(".gguf");
    if (dotPos != std::string::npos) {
        path = path.substr(0, dotPos);
    }
    return path.empty() ? "unknown" : path;
}

size_t LlamaInference::getModelSize() const {
    if (config_.modelPath.empty()) return 0;
    
    std::ifstream file(config_.modelPath, std::ios::binary | std::ios::ate);
    if (!file.good()) return 0;
    return static_cast<size_t>(file.tellg());
}

std::string LlamaInference::buildPrompt(const std::string& systemPrompt,
                                         const std::vector<Conversation>& conversations) {
    std::ostringstream prompt;
    
    // UI-TARS specific prompt format (ChatML style)
    prompt << "<|im_start|>system\n";
    prompt << systemPrompt << "\n";
    prompt << "<|im_end|>\n";
    
    for (const auto& conv : conversations) {
        if (conv.from == "human") {
            prompt << "<|im_start|>user\n";
            if (!conv.screenshotBase64.empty()) {
                prompt << "<image>\n";
            }
            prompt << conv.value << "\n";
            prompt << "<|im_end|>\n";
        } else if (conv.from == "gpt") {
            prompt << "<|im_start|>assistant\n";
            prompt << conv.value << "\n";
            prompt << "<|im_end|>\n";
        }
    }
    
    // Start assistant response
    prompt << "<|im_start|>assistant\n";
    
    return prompt.str();
}

std::vector<float> LlamaInference::processImage(const std::string& base64Image) {
    std::vector<float> embeddings;
    
    // Decode base64 image
    std::string imageData = base64Image;
    size_t commaPos = imageData.find(",");
    if (commaPos != std::string::npos) {
        imageData = imageData.substr(commaPos + 1);
    }
    
    std::vector<uint8_t> decoded = base64_decode(imageData);
    if (decoded.empty()) {
        logger_->error("Failed to decode base64 image");
        return embeddings;
    }
    
    logger_->info("Image decoded, size: " + std::to_string(decoded.size()) + " bytes");
    
    // TODO: Process through CLIP when llama.cpp is integrated
    // For now, return empty embeddings
    
    return embeddings;
}

std::vector<int32_t> LlamaInference::tokenize(const std::string& text, bool addBos) {
    // Placeholder tokenization
    // In actual implementation, this uses llama_tokenize
    std::vector<int32_t> tokens;
    
    // Simple word-based tokenization as placeholder
    std::istringstream iss(text);
    std::string word;
    int32_t tokenId = addBos ? 1 : 0;  // BOS token
    
    while (iss >> word) {
        tokens.push_back(tokenId++);
    }
    
    return tokens;
}

int32_t LlamaInference::sampleToken() {
    // Placeholder - returns EOS
    return 0;
}

std::vector<ParsedAction> LlamaInference::parseActions(const std::string& response) {
    std::vector<ParsedAction> actions;
    
    std::istringstream ss(response);
    std::string line;
    
    while (std::getline(ss, line)) {
        if (line.find("Action:") != std::string::npos || 
            line.find("action:") != std::string::npos) {
            size_t pos = line.find(":");
            if (pos != std::string::npos) {
                std::string actionStr = line.substr(pos + 1);
                actionStr.erase(0, actionStr.find_first_not_of(" \t"));
                actionStr.erase(actionStr.find_last_not_of(" \t") + 1);
                
                ParsedAction action;
                
                size_t parenPos = actionStr.find("(");
                if (parenPos != std::string::npos) {
                    action.actionTypeStr = actionStr.substr(0, parenPos);
                    action.actionType = stringToActionType(action.actionTypeStr);
                    
                    size_t closeParenPos = actionStr.rfind(")");
                    if (closeParenPos > parenPos) {
                        std::string params = actionStr.substr(parenPos + 1, closeParenPos - parenPos - 1);
                        
                        std::regex paramRegex(R"((\w+)\s*=\s*'([^']*)')");
                        std::smatch match;
                        std::string::const_iterator searchStart(params.cbegin());
                        
                        while (std::regex_search(searchStart, params.cend(), match, paramRegex)) {
                            action.actionInputs[match[1].str()] = match[2].str();
                            searchStart = match.suffix().first;
                        }
                    }
                } else {
                    action.actionTypeStr = actionStr;
                    action.actionType = stringToActionType(actionStr);
                }
                
                actions.push_back(action);
            }
        }
    }
    
    return actions;
}

VLMResponse LlamaInference::generate(const std::string& systemPrompt,
                                      const std::vector<Conversation>& conversations,
                                      int screenWidth,
                                      int screenHeight,
                                      double scaleFactor,
                                      TokenCallback callback) {
    auto startTime = std::chrono::steady_clock::now();
    
    VLMResponse response;
    response.costTokens = 0;
    response.costTime = 0;
    
    if (!modelLoaded_) {
        throw std::runtime_error("Model not loaded");
    }
    
    // Build prompt
    std::string prompt = buildPrompt(systemPrompt, conversations);
    logger_->debug("Prompt length: " + std::to_string(prompt.length()));
    
    // Process any images in the conversation
    for (const auto& conv : conversations) {
        if (!conv.screenshotBase64.empty()) {
            processImage(conv.screenshotBase64);
            break;
        }
    }
    
#ifdef HAS_LLAMA
    // TODO: Actual inference with llama.cpp when ENABLE_LLAMA is set
    throw std::runtime_error("llama.cpp inference not yet implemented. "
                            "Use API providers (openai, anthropic) or wait for full llama.cpp integration.");
#else
    // Stub response for builds without llama.cpp
    logger_->warn("Using stub response - build with ENABLE_LLAMA=ON for real inference");
    response.prediction = "Thought: [STUB] llama.cpp not enabled. This is a placeholder response.\n"
                         "Action: wait(duration='1')";
    response.parsedPredictions = parseActions(response.prediction);
    response.costTokens = 50;
#endif
    
    auto endTime = std::chrono::steady_clock::now();
    response.costTime = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();
    
    logger_->info("Generated response in " + std::to_string(response.costTime) + "ms");
    
    return response;
}

} // namespace uitars
