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
    std::string provider;      // e.g., "openai", "anthropic", "volcengine", "local"
    std::string model;         // e.g., "gpt-4-vision-preview" or path to GGUF
    std::string apiKey;
    std::string baseUrl;       // API endpoint
    double temperature = 0.0;
    int maxTokens = 4096;
};

struct LocalModelConfig {
    std::string modelPath;     // Path to main GGUF model
    std::string mmprojPath;    // Path to multimodal projector GGUF
    int nCtx = 4096;           // Context size
    int nBatch = 512;          // Batch size  
    int nThreads = 4;          // Number of threads
    int nGpuLayers = 0;        // GPU layers (0 = CPU only)
    bool useFlashAttn = false; // Use flash attention
    bool useMmap = true;       // Memory-map model
    bool useMlock = false;     // Lock model in memory
};

struct Config {
    // Model configuration
    ModelConfig model;
    
    // Local model configuration (for embedded inference)
    LocalModelConfig localModel;
    
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
    
    // Check if using local model
    bool isLocalModel() const { return model.provider == "local"; }
};

} // namespace uitars

#endif // UI_TARS_CONFIG_H
