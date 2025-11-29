/*
 * Copyright (c) 2025 Bytedance, Inc. and its affiliates.
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef UI_TARS_CONFIG_H
#define UI_TARS_CONFIG_H

#include <string>
#include <vector>

namespace uitars {

struct ModelConfig {
    std::string provider;      // e.g., "openai", "anthropic", "volcengine"
    std::string model;         // e.g., "gpt-4-vision-preview"
    std::string apiKey;
    std::string baseUrl;       // API endpoint
    double temperature = 0.0;
    int maxTokens = 4096;
};

struct Config {
    // Model configuration
    ModelConfig model;
    
    // Agent configuration
    int maxLoopCount = 50;
    int loopIntervalMs = 1000;
    
    // Retry configuration
    int screenshotRetries = 3;
    int modelRetries = 3;
    int executeRetries = 3;
    
    // Network configuration
    int apiTimeoutSeconds = 120;  // Timeout for VLM API calls
    
    // Service configuration
    bool runAsService = false;
    std::string socketPath = "/var/run/ui-tars-agent.sock";
    std::string logPath = "/var/log/ui-tars-agent.log";
    std::string configPath = "/etc/ui-tars/ui-tars-agent.conf";
    
    // Display configuration
    std::string displayEnv = "";  // e.g., ":0" for X11
    
    // Load configuration from file
    static Config loadFromFile(const std::string& path);
    
    // Load configuration from environment variables
    static Config loadFromEnv();
    
    // Save configuration to file
    void saveToFile(const std::string& path) const;
    
    // Validate configuration
    bool validate() const;
};

} // namespace uitars

#endif // UI_TARS_CONFIG_H
