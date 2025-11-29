/*
 * Copyright (c) 2025 Bytedance, Inc. and its affiliates.
 * SPDX-License-Identifier: Apache-2.0
 */
#include "vlm_client.h"
#include <nlohmann/json.hpp>
#include <sstream>
#include <chrono>

using json = nlohmann::json;

namespace uitars {

VLMClient::VLMClient(const Config& config, std::shared_ptr<Logger> logger) 
    : config_(config), logger_(logger), curl_(nullptr) {
    if (!initCurl()) {
        throw std::runtime_error("Failed to initialize CURL");
    }
}

VLMClient::~VLMClient() {
    cleanup();
}

bool VLMClient::initCurl() {
    curl_ = curl_easy_init();
    if (!curl_) {
        logger_->error("Failed to initialize CURL");
        return false;
    }
    return true;
}

void VLMClient::cleanup() {
    if (curl_) {
        curl_easy_cleanup(curl_);
        curl_ = nullptr;
    }
}

size_t VLMClient::writeCallback(void* contents, size_t size, size_t nmemb, std::string* output) {
    size_t totalSize = size * nmemb;
    output->append(static_cast<char*>(contents), totalSize);
    return totalSize;
}

std::string VLMClient::getModelName() const {
    return config_.model.model;
}

void VLMClient::reset() {
    previousResponseId_.clear();
}

std::string VLMClient::buildRequestBody(
    const std::string& systemPrompt,
    const std::vector<Conversation>& conversations,
    int screenWidth,
    int screenHeight
) {
    json messages = json::array();
    
    // Add system message
    messages.push_back({
        {"role", "system"},
        {"content", systemPrompt}
    });
    
    // Build conversation history
    for (const auto& conv : conversations) {
        json message;
        
        if (conv.from == "human") {
            message["role"] = "user";
            
            if (!conv.screenshotBase64.empty()) {
                // Image message
                json content = json::array();
                content.push_back({
                    {"type", "image_url"},
                    {"image_url", {
                        {"url", "data:image/png;base64," + conv.screenshotBase64}
                    }}
                });
                content.push_back({
                    {"type", "text"},
                    {"text", conv.value}
                });
                message["content"] = content;
            } else {
                message["content"] = conv.value;
            }
        } else if (conv.from == "gpt") {
            message["role"] = "assistant";
            message["content"] = conv.value;
        }
        
        messages.push_back(message);
    }
    
    json requestBody = {
        {"model", config_.model.model},
        {"messages", messages},
        {"max_tokens", config_.model.maxTokens},
        {"temperature", config_.model.temperature}
    };
    
    return requestBody.dump();
}

VLMResponse VLMClient::invoke(
    const std::string& systemPrompt,
    const std::vector<Conversation>& conversations,
    int screenWidth,
    int screenHeight,
    double scaleFactor
) {
    auto startTime = std::chrono::steady_clock::now();
    
    VLMResponse response;
    response.costTokens = 0;
    response.costTime = 0;
    
    if (!curl_) {
        throw std::runtime_error("CURL not initialized");
    }
    
    // Determine API endpoint
    std::string apiUrl = config_.model.baseUrl;
    if (apiUrl.empty()) {
        if (config_.model.provider == "openai") {
            apiUrl = "https://api.openai.com/v1/chat/completions";
        } else if (config_.model.provider == "anthropic") {
            apiUrl = "https://api.anthropic.com/v1/messages";
        } else if (config_.model.provider == "volcengine") {
            apiUrl = "https://ark.cn-beijing.volces.com/api/v3/chat/completions";
        } else {
            throw std::runtime_error("Unknown provider: " + config_.model.provider);
        }
    }
    
    // Build request body
    std::string requestBody = buildRequestBody(systemPrompt, conversations, screenWidth, screenHeight);
    
    logger_->debug("Request to: " + apiUrl);
    logger_->debug("Request body length: " + std::to_string(requestBody.length()));
    
    // Set up CURL
    curl_easy_reset(curl_);
    
    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, "Content-Type: application/json");
    headers = curl_slist_append(headers, ("Authorization: Bearer " + config_.model.apiKey).c_str());
    
    if (config_.model.provider == "anthropic") {
        headers = curl_slist_append(headers, ("x-api-key: " + config_.model.apiKey).c_str());
        headers = curl_slist_append(headers, "anthropic-version: 2023-06-01");
    }
    
    std::string responseBody;
    
    curl_easy_setopt(curl_, CURLOPT_URL, apiUrl.c_str());
    curl_easy_setopt(curl_, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl_, CURLOPT_POSTFIELDS, requestBody.c_str());
    curl_easy_setopt(curl_, CURLOPT_WRITEFUNCTION, writeCallback);
    curl_easy_setopt(curl_, CURLOPT_WRITEDATA, &responseBody);
    curl_easy_setopt(curl_, CURLOPT_TIMEOUT, 120L);  // 2 minute timeout
    curl_easy_setopt(curl_, CURLOPT_SSL_VERIFYPEER, 1L);
    
    CURLcode res = curl_easy_perform(curl_);
    
    curl_slist_free_all(headers);
    
    if (res != CURLE_OK) {
        throw std::runtime_error("CURL request failed: " + std::string(curl_easy_strerror(res)));
    }
    
    long httpCode;
    curl_easy_getinfo(curl_, CURLINFO_RESPONSE_CODE, &httpCode);
    
    if (httpCode != 200) {
        logger_->error("API returned HTTP " + std::to_string(httpCode) + ": " + responseBody);
        throw std::runtime_error("API request failed with HTTP " + std::to_string(httpCode));
    }
    
    // Parse response
    response = parseResponse(responseBody);
    
    auto endTime = std::chrono::steady_clock::now();
    response.costTime = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();
    
    logger_->info("VLM response received in " + std::to_string(response.costTime) + "ms");
    
    return response;
}

VLMResponse VLMClient::parseResponse(const std::string& responseStr) {
    VLMResponse response;
    
    try {
        json responseJson = json::parse(responseStr);
        
        // Parse based on provider format
        if (config_.model.provider == "anthropic") {
            // Anthropic format
            if (responseJson.contains("content") && responseJson["content"].is_array() && !responseJson["content"].empty()) {
                response.prediction = responseJson["content"][0]["text"].get<std::string>();
            }
            if (responseJson.contains("id")) {
                response.responseId = responseJson["id"].get<std::string>();
            }
            if (responseJson.contains("usage")) {
                response.costTokens = responseJson["usage"]["output_tokens"].get<int>();
            }
        } else {
            // OpenAI-compatible format
            if (responseJson.contains("choices") && responseJson["choices"].is_array() && !responseJson["choices"].empty()) {
                response.prediction = responseJson["choices"][0]["message"]["content"].get<std::string>();
            }
            if (responseJson.contains("id")) {
                response.responseId = responseJson["id"].get<std::string>();
            }
            if (responseJson.contains("usage")) {
                response.costTokens = responseJson["usage"]["total_tokens"].get<int>();
            }
        }
        
        // Parse actions from prediction
        if (!response.prediction.empty()) {
            // Simple parsing - look for Action: lines
            std::istringstream ss(response.prediction);
            std::string line;
            
            while (std::getline(ss, line)) {
                if (line.find("Action:") != std::string::npos || 
                    line.find("action:") != std::string::npos) {
                    // Extract action part
                    size_t pos = line.find(":");
                    if (pos != std::string::npos) {
                        std::string actionStr = line.substr(pos + 1);
                        // Trim whitespace
                        actionStr.erase(0, actionStr.find_first_not_of(" \t"));
                        actionStr.erase(actionStr.find_last_not_of(" \t") + 1);
                        
                        ParsedAction action;
                        
                        // Parse action type and inputs
                        size_t parenPos = actionStr.find("(");
                        if (parenPos != std::string::npos) {
                            action.actionTypeStr = actionStr.substr(0, parenPos);
                            action.actionType = stringToActionType(action.actionTypeStr);
                            
                            // Extract parameters
                            size_t closeParenPos = actionStr.rfind(")");
                            if (closeParenPos > parenPos) {
                                std::string params = actionStr.substr(parenPos + 1, closeParenPos - parenPos - 1);
                                
                                // Parse key=value pairs
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
                        
                        response.parsedPredictions.push_back(action);
                    }
                }
            }
        }
        
    } catch (const json::exception& e) {
        throw std::runtime_error("Failed to parse VLM response: " + std::string(e.what()));
    }
    
    return response;
}

} // namespace uitars
