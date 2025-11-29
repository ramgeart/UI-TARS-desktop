/*
 * Copyright (c) 2025 Bytedance, Inc. and its affiliates.
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef UI_TARS_LLAMA_INFERENCE_H
#define UI_TARS_LLAMA_INFERENCE_H

#include <string>
#include <vector>
#include <memory>
#include <functional>

#include "types.h"
#include "config.h"
#include "logger.h"

namespace uitars {

// Callback for streaming tokens
using TokenCallback = std::function<void(const std::string& token)>;

class LlamaInference {
public:
    LlamaInference(const LocalModelConfig& config, std::shared_ptr<Logger> logger);
    ~LlamaInference();

    // Initialize and load model
    bool loadModel();
    
    // Unload model and free resources
    void unloadModel();
    
    // Check if model is loaded
    bool isLoaded() const;

    // Generate response from prompt with optional image
    VLMResponse generate(const std::string& systemPrompt,
                         const std::vector<Conversation>& conversations,
                         int screenWidth,
                         int screenHeight,
                         double scaleFactor,
                         TokenCallback callback = nullptr);

    // Get model info
    std::string getModelName() const;
    size_t getModelSize() const;

private:
    LocalModelConfig config_;
    std::shared_ptr<Logger> logger_;
    
    bool modelLoaded_ = false;

    // Build prompt from conversations
    std::string buildPrompt(const std::string& systemPrompt,
                           const std::vector<Conversation>& conversations);
    
    // Process image for multimodal input
    std::vector<float> processImage(const std::string& base64Image);
    
    // Tokenize text
    std::vector<int32_t> tokenize(const std::string& text, bool addBos);
    
    // Sample next token
    int32_t sampleToken();
    
    // Parse response into actions
    std::vector<ParsedAction> parseActions(const std::string& response);
};

} // namespace uitars

#endif // UI_TARS_LLAMA_INFERENCE_H
