/*
 * Copyright (c) 2025 Bytedance, Inc. and its affiliates.
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef UI_TARS_SERVICE_H
#define UI_TARS_SERVICE_H

#include <string>
#include <memory>
#include <atomic>
#include <thread>
#include <functional>

#include "gui_agent.h"
#include "config.h"
#include "logger.h"

namespace uitars {

class Service {
public:
    Service(const Config& config);
    ~Service();

    // Start the service
    bool start();
    
    // Stop the service
    void stop();
    
    // Check if service is running
    bool isRunning() const;
    
    // Run as daemon
    static bool daemonize();
    
    // Signal handlers
    static void setupSignalHandlers();
    static void signalHandler(int signum);

private:
    Config config_;
    std::shared_ptr<Logger> logger_;
    std::unique_ptr<GUIAgent> agent_;
    std::atomic<bool> running_{false};
    std::thread serviceThread_;
    int socketFd_{-1};
    
    static std::atomic<bool> shouldStop_;
    
    bool createSocket();
    void handleConnections();
    void processCommand(int clientFd, const std::string& command);
    void cleanup();
};

} // namespace uitars

#endif // UI_TARS_SERVICE_H
