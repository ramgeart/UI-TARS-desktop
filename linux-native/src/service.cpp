/*
 * Copyright (c) 2025 Bytedance, Inc. and its affiliates.
 * SPDX-License-Identifier: Apache-2.0
 */
#include "service.h"
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/stat.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <cstring>
#include <sstream>

namespace uitars {

std::atomic<bool> Service::shouldStop_{false};

Service::Service(const Config& config) 
    : config_(config), socketFd_(-1) {
    logger_ = Logger::getInstance();
    if (!logger_) {
        logger_ = std::make_shared<Logger>(config.logPath);
    }
}

Service::~Service() {
    stop();
    cleanup();
}

bool Service::daemonize() {
    pid_t pid = fork();
    
    if (pid < 0) {
        return false;
    }
    
    if (pid > 0) {
        // Parent process exits
        exit(0);
    }
    
    // Child process continues
    
    // Create new session
    if (setsid() < 0) {
        return false;
    }
    
    // Ignore signals
    signal(SIGCHLD, SIG_IGN);
    signal(SIGHUP, SIG_IGN);
    
    // Fork again to prevent acquiring a controlling terminal
    pid = fork();
    if (pid < 0) {
        return false;
    }
    if (pid > 0) {
        exit(0);
    }
    
    // Set file permissions
    umask(0);
    
    // Change to root directory
    chdir("/");
    
    // Close standard file descriptors
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);
    
    // Redirect to /dev/null
    open("/dev/null", O_RDONLY);  // stdin
    open("/dev/null", O_RDWR);    // stdout
    open("/dev/null", O_RDWR);    // stderr
    
    return true;
}

void Service::setupSignalHandlers() {
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);
    signal(SIGHUP, SIG_IGN);
}

void Service::signalHandler(int signum) {
    shouldStop_ = true;
}

bool Service::start() {
    if (running_) {
        logger_->warn("Service is already running");
        return false;
    }
    
    setupSignalHandlers();
    
    if (!createSocket()) {
        return false;
    }
    
    running_ = true;
    shouldStop_ = false;
    
    logger_->info("Service started, listening on " + config_.socketPath);
    
    // Run in separate thread
    serviceThread_ = std::thread([this]() {
        handleConnections();
    });
    
    return true;
}

void Service::stop() {
    shouldStop_ = true;
    running_ = false;
    
    // Close socket to unblock accept()
    if (socketFd_ >= 0) {
        shutdown(socketFd_, SHUT_RDWR);
        close(socketFd_);
        socketFd_ = -1;
    }
    
    if (serviceThread_.joinable()) {
        serviceThread_.join();
    }
    
    logger_->info("Service stopped");
}

bool Service::isRunning() const {
    return running_ && !shouldStop_;
}

bool Service::createSocket() {
    // Remove existing socket file (ignore errors if file doesn't exist)
    if (unlink(config_.socketPath.c_str()) < 0 && errno != ENOENT) {
        logger_->warn("Could not remove existing socket file: " + std::string(strerror(errno)));
    }
    
    socketFd_ = socket(AF_UNIX, SOCK_STREAM, 0);
    if (socketFd_ < 0) {
        logger_->error("Failed to create socket: " + std::string(strerror(errno)));
        return false;
    }
    
    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, config_.socketPath.c_str(), sizeof(addr.sun_path) - 1);
    
    if (bind(socketFd_, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        logger_->error("Failed to bind socket: " + std::string(strerror(errno)));
        close(socketFd_);
        socketFd_ = -1;
        return false;
    }
    
    // Set permissions
    chmod(config_.socketPath.c_str(), 0660);
    
    if (listen(socketFd_, 5) < 0) {
        logger_->error("Failed to listen on socket: " + std::string(strerror(errno)));
        close(socketFd_);
        socketFd_ = -1;
        return false;
    }
    
    return true;
}

void Service::handleConnections() {
    while (running_ && !shouldStop_) {
        struct sockaddr_un clientAddr;
        socklen_t clientLen = sizeof(clientAddr);
        
        int clientFd = accept(socketFd_, (struct sockaddr*)&clientAddr, &clientLen);
        
        if (clientFd < 0) {
            if (shouldStop_ || !running_) {
                break;
            }
            logger_->error("Failed to accept connection: " + std::string(strerror(errno)));
            continue;
        }
        
        logger_->debug("Client connected");
        
        // Read command
        char buffer[4096];
        memset(buffer, 0, sizeof(buffer));
        ssize_t n = read(clientFd, buffer, sizeof(buffer) - 1);
        
        if (n < 0) {
            logger_->error("Failed to read from client: " + std::string(strerror(errno)));
            close(clientFd);
            continue;
        }
        
        if (n > 0) {
            buffer[n] = '\0';
            std::string command(buffer);
            
            // Trim newline
            if (!command.empty() && command.back() == '\n') {
                command.pop_back();
            }
            
            processCommand(clientFd, command);
        }
        
        close(clientFd);
    }
}

void Service::processCommand(int clientFd, const std::string& command) {
    logger_->info("Received command: " + command);
    
    std::string response;
    
    if (command == "status") {
        if (agent_ && agent_->isRunning()) {
            if (agent_->isPaused()) {
                response = "paused\n";
            } else {
                response = "running\n";
            }
        } else {
            response = "idle\n";
        }
    } else if (command == "pause") {
        if (agent_ && agent_->isRunning()) {
            agent_->pause();
            response = "paused\n";
        } else {
            response = "error: not running\n";
        }
    } else if (command == "resume") {
        if (agent_ && agent_->isPaused()) {
            agent_->resume();
            response = "resumed\n";
        } else {
            response = "error: not paused\n";
        }
    } else if (command == "stop") {
        if (agent_ && agent_->isRunning()) {
            agent_->stop();
            response = "stopped\n";
        } else {
            response = "ok\n";
        }
    } else if (command.substr(0, 4) == "run ") {
        std::string instruction = command.substr(4);
        
        if (agent_ && agent_->isRunning()) {
            response = "error: agent already running\n";
        } else {
            // Create new agent and run instruction
            agent_ = std::make_unique<GUIAgent>(config_);
            
            // Run asynchronously
            std::thread([this, instruction]() {
                agent_->run(instruction);
            }).detach();
            
            response = "started\n";
        }
    } else if (command == "help") {
        response = "Commands:\n";
        response += "  run <instruction>  - Run an instruction\n";
        response += "  status             - Get agent status\n";
        response += "  pause              - Pause execution\n";
        response += "  resume             - Resume execution\n";
        response += "  stop               - Stop execution\n";
        response += "  help               - Show this help\n";
    } else {
        response = "error: unknown command\n";
    }
    
    write(clientFd, response.c_str(), response.length());
}

void Service::cleanup() {
    if (socketFd_ >= 0) {
        close(socketFd_);
        socketFd_ = -1;
    }
    
    unlink(config_.socketPath.c_str());
}

} // namespace uitars
