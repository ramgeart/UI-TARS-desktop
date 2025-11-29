/*
 * Copyright (c) 2025 Bytedance, Inc. and its affiliates.
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef UI_TARS_VLM_CLIENT_H
#define UI_TARS_VLM_CLIENT_H

#include <string>
#include <vector>
#include <memory>
#include <curl/curl.h>

#include "types.h"
#include "config.h"
#include "logger.h"

namespace uitars {

class VLMClient {
public:
    VLMClient(const Config& config, std::shared_ptr<Logger> logger);
    ~VLMClient();

    // Invoke the VLM model
    VLMResponse invoke(const std::string& systemPrompt,
                       const std::vector<Conversation>& conversations,
                       int screenWidth,
                       int screenHeight,
                       double scaleFactor);

    // Reset client state
    void reset();

    // Get model name
    std::string getModelName() const;

private:
    Config config_;
    std::shared_ptr<Logger> logger_;
    CURL* curl_;
    std::string previousResponseId_;

    bool initCurl();
    void cleanup();
    
    std::string buildRequestBody(const std::string& systemPrompt,
                                  const std::vector<Conversation>& conversations,
                                  int screenWidth,
                                  int screenHeight);
    
    VLMResponse parseResponse(const std::string& response);
    
    static size_t writeCallback(void* contents, size_t size, size_t nmemb, std::string* output);
};

} // namespace uitars

#endif // UI_TARS_VLM_CLIENT_H
