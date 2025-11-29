/*
 * Copyright (c) 2025 Bytedance, Inc. and its affiliates.
 * SPDX-License-Identifier: Apache-2.0
 */
#include "config.h"
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <regex>
#include <iostream>

namespace uitars {

Config Config::loadFromFile(const std::string& path) {
    Config config;
    
    std::ifstream file(path);
    if (!file.is_open()) {
        std::cerr << "Warning: Could not open config file: " << path << std::endl;
        return config;
    }
    
    std::string line;
    while (std::getline(file, line)) {
        // Skip comments and empty lines
        if (line.empty() || line[0] == '#' || line[0] == ';') {
            continue;
        }
        
        // Remove inline comments
        size_t commentPos = line.find('#');
        if (commentPos != std::string::npos) {
            line = line.substr(0, commentPos);
        }
        
        // Parse key=value
        size_t equalPos = line.find('=');
        if (equalPos == std::string::npos) {
            continue;
        }
        
        std::string key = line.substr(0, equalPos);
        std::string value = line.substr(equalPos + 1);
        
        // Trim whitespace
        key.erase(0, key.find_first_not_of(" \t"));
        key.erase(key.find_last_not_of(" \t") + 1);
        value.erase(0, value.find_first_not_of(" \t\"'"));
        value.erase(value.find_last_not_of(" \t\"'") + 1);
        
        // Map to config fields
        if (key == "provider") {
            config.model.provider = value;
        } else if (key == "model") {
            config.model.model = value;
        } else if (key == "api_key" || key == "apiKey") {
            config.model.apiKey = value;
        } else if (key == "base_url" || key == "baseUrl") {
            config.model.baseUrl = value;
        } else if (key == "temperature") {
            config.model.temperature = std::stod(value);
        } else if (key == "max_tokens" || key == "maxTokens") {
            config.model.maxTokens = std::stoi(value);
        } else if (key == "max_loop_count" || key == "maxLoopCount") {
            config.maxLoopCount = std::stoi(value);
        } else if (key == "loop_interval_ms" || key == "loopIntervalMs") {
            config.loopIntervalMs = std::stoi(value);
        } else if (key == "screenshot_retries" || key == "screenshotRetries") {
            config.screenshotRetries = std::stoi(value);
        } else if (key == "model_retries" || key == "modelRetries") {
            config.modelRetries = std::stoi(value);
        } else if (key == "execute_retries" || key == "executeRetries") {
            config.executeRetries = std::stoi(value);
        } else if (key == "api_timeout" || key == "apiTimeoutSeconds") {
            config.apiTimeoutSeconds = std::stoi(value);
        } else if (key == "socket_path" || key == "socketPath") {
            config.socketPath = value;
        } else if (key == "log_path" || key == "logPath") {
            config.logPath = value;
        } else if (key == "display") {
            config.displayEnv = value;
        // Local model configuration
        } else if (key == "model_path" || key == "modelPath") {
            config.localModel.modelPath = value;
        } else if (key == "mmproj_path" || key == "mmprojPath") {
            config.localModel.mmprojPath = value;
        } else if (key == "n_ctx" || key == "nCtx") {
            config.localModel.nCtx = std::stoi(value);
        } else if (key == "n_batch" || key == "nBatch") {
            config.localModel.nBatch = std::stoi(value);
        } else if (key == "n_threads" || key == "nThreads") {
            config.localModel.nThreads = std::stoi(value);
        } else if (key == "n_gpu_layers" || key == "nGpuLayers") {
            config.localModel.nGpuLayers = std::stoi(value);
        } else if (key == "use_flash_attn" || key == "useFlashAttn") {
            config.localModel.useFlashAttn = (value == "true" || value == "1");
        } else if (key == "use_mmap" || key == "useMmap") {
            config.localModel.useMmap = (value == "true" || value == "1");
        } else if (key == "use_mlock" || key == "useMlock") {
            config.localModel.useMlock = (value == "true" || value == "1");
        }
    }
    
    file.close();
    return config;
}

Config Config::loadFromEnv() {
    Config config;
    
    // Model configuration from environment
    const char* provider = std::getenv("UI_TARS_PROVIDER");
    if (provider) config.model.provider = provider;
    
    const char* model = std::getenv("UI_TARS_MODEL");
    if (model) config.model.model = model;
    
    const char* apiKey = std::getenv("UI_TARS_API_KEY");
    if (apiKey) config.model.apiKey = apiKey;
    
    const char* baseUrl = std::getenv("UI_TARS_BASE_URL");
    if (baseUrl) config.model.baseUrl = baseUrl;
    
    const char* temperature = std::getenv("UI_TARS_TEMPERATURE");
    if (temperature) config.model.temperature = std::stod(temperature);
    
    const char* maxTokens = std::getenv("UI_TARS_MAX_TOKENS");
    if (maxTokens) config.model.maxTokens = std::stoi(maxTokens);
    
    // Agent configuration
    const char* maxLoopCount = std::getenv("UI_TARS_MAX_LOOP_COUNT");
    if (maxLoopCount) config.maxLoopCount = std::stoi(maxLoopCount);
    
    const char* loopIntervalMs = std::getenv("UI_TARS_LOOP_INTERVAL_MS");
    if (loopIntervalMs) config.loopIntervalMs = std::stoi(loopIntervalMs);
    
    // Network configuration
    const char* apiTimeout = std::getenv("UI_TARS_API_TIMEOUT");
    if (apiTimeout) config.apiTimeoutSeconds = std::stoi(apiTimeout);
    
    // Service configuration
    const char* socketPath = std::getenv("UI_TARS_SOCKET_PATH");
    if (socketPath) config.socketPath = socketPath;
    
    const char* logPath = std::getenv("UI_TARS_LOG_PATH");
    if (logPath) config.logPath = logPath;
    
    // Display
    const char* display = std::getenv("DISPLAY");
    if (display) config.displayEnv = display;
    
    // Local model configuration from environment
    const char* modelPath = std::getenv("UI_TARS_MODEL_PATH");
    if (modelPath) config.localModel.modelPath = modelPath;
    
    const char* mmprojPath = std::getenv("UI_TARS_MMPROJ_PATH");
    if (mmprojPath) config.localModel.mmprojPath = mmprojPath;
    
    const char* nCtx = std::getenv("UI_TARS_N_CTX");
    if (nCtx) config.localModel.nCtx = std::stoi(nCtx);
    
    const char* nThreads = std::getenv("UI_TARS_N_THREADS");
    if (nThreads) config.localModel.nThreads = std::stoi(nThreads);
    
    const char* nGpuLayers = std::getenv("UI_TARS_N_GPU_LAYERS");
    if (nGpuLayers) config.localModel.nGpuLayers = std::stoi(nGpuLayers);
    
    return config;
}

void Config::saveToFile(const std::string& path) const {
    std::ofstream file(path);
    if (!file.is_open()) {
        throw std::runtime_error("Could not open config file for writing: " + path);
    }
    
    file << "# UI-TARS Linux Agent Configuration\n";
    file << "\n";
    file << "# Model Configuration\n";
    file << "# Use provider = local for embedded inference with GGUF models\n";
    file << "provider = " << model.provider << "\n";
    file << "model = " << model.model << "\n";
    if (!model.apiKey.empty()) {
        file << "api_key = " << model.apiKey << "\n";
    }
    if (!model.baseUrl.empty()) {
        file << "base_url = " << model.baseUrl << "\n";
    }
    file << "temperature = " << model.temperature << "\n";
    file << "max_tokens = " << model.maxTokens << "\n";
    file << "\n";
    file << "# Local Model Configuration (for provider = local)\n";
    file << "# Download GGUF models from:\n";
    file << "# https://huggingface.co/mradermacher/UI-TARS-1.5-7B-i1-GGUF\n";
    file << "# https://huggingface.co/mradermacher/UI-TARS-1.5-7B-GGUF\n";
    if (!localModel.modelPath.empty()) {
        file << "model_path = " << localModel.modelPath << "\n";
    } else {
        file << "# model_path = /path/to/UI-TARS-1.5-7B.i1-Q4_K_S.gguf\n";
    }
    if (!localModel.mmprojPath.empty()) {
        file << "mmproj_path = " << localModel.mmprojPath << "\n";
    } else {
        file << "# mmproj_path = /path/to/UI-TARS-1.5-7B.mmproj-Q8_0.gguf\n";
    }
    file << "n_ctx = " << localModel.nCtx << "\n";
    file << "n_batch = " << localModel.nBatch << "\n";
    file << "n_threads = " << localModel.nThreads << "\n";
    file << "n_gpu_layers = " << localModel.nGpuLayers << "\n";
    file << "\n";
    file << "# Agent Configuration\n";
    file << "max_loop_count = " << maxLoopCount << "\n";
    file << "loop_interval_ms = " << loopIntervalMs << "\n";
    file << "\n";
    file << "# Retry Configuration\n";
    file << "screenshot_retries = " << screenshotRetries << "\n";
    file << "model_retries = " << modelRetries << "\n";
    file << "execute_retries = " << executeRetries << "\n";
    file << "\n";
    file << "# Service Configuration\n";
    file << "socket_path = " << socketPath << "\n";
    file << "log_path = " << logPath << "\n";
    if (!displayEnv.empty()) {
        file << "display = " << displayEnv << "\n";
    }
    
    file.close();
}

bool Config::validate() const {
    // Check required model configuration
    if (model.provider.empty()) {
        std::cerr << "Error: Model provider is required" << std::endl;
        return false;
    }
    
    // For local provider, check model path
    if (model.provider == "local") {
        if (localModel.modelPath.empty()) {
            std::cerr << "Error: model_path is required for local provider" << std::endl;
            return false;
        }
        // API key not required for local
        return true;
    }
    
    if (model.model.empty()) {
        std::cerr << "Error: Model name is required" << std::endl;
        return false;
    }
    
    if (model.apiKey.empty()) {
        std::cerr << "Error: API key is required" << std::endl;
        return false;
    }
    
    // Validate provider
    if (model.provider != "openai" && 
        model.provider != "anthropic" && 
        model.provider != "volcengine" &&
        model.provider != "local" &&
        model.baseUrl.empty()) {
        std::cerr << "Warning: Unknown provider '" << model.provider 
                  << "' - base_url is required" << std::endl;
    }
    
    // Validate numeric values
    if (maxLoopCount <= 0 || maxLoopCount > 1000) {
        std::cerr << "Warning: max_loop_count should be between 1 and 1000" << std::endl;
    }
    
    if (loopIntervalMs < 0) {
        std::cerr << "Warning: loop_interval_ms should be non-negative" << std::endl;
    }
    
    return true;
}

} // namespace uitars
