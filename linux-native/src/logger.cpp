/*
 * Copyright (c) 2025 Bytedance, Inc. and its affiliates.
 * SPDX-License-Identifier: Apache-2.0
 */
#include "logger.h"
#include <iostream>
#include <iomanip>
#include <ctime>
#include <sstream>

namespace uitars {

std::shared_ptr<Logger> Logger::instance_ = nullptr;
std::mutex Logger::instanceMutex_;

Logger::Logger(const std::string& logPath, LogLevel level) 
    : logPath_(logPath), level_(level) {
    if (!logPath_.empty()) {
        logFile_.open(logPath_, std::ios::app);
        if (!logFile_.is_open()) {
            std::cerr << "Warning: Could not open log file: " << logPath_ << std::endl;
        }
    }
}

Logger::~Logger() {
    if (logFile_.is_open()) {
        logFile_.close();
    }
}

std::shared_ptr<Logger> Logger::getInstance() {
    std::lock_guard<std::mutex> lock(instanceMutex_);
    if (!instance_) {
        instance_ = std::make_shared<Logger>();
    }
    return instance_;
}

void Logger::setInstance(std::shared_ptr<Logger> logger) {
    std::lock_guard<std::mutex> lock(instanceMutex_);
    instance_ = logger;
}

void Logger::setLevel(LogLevel level) {
    level_ = level;
}

void Logger::setLogPath(const std::string& path) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (logFile_.is_open()) {
        logFile_.close();
    }
    
    logPath_ = path;
    
    if (!logPath_.empty()) {
        logFile_.open(logPath_, std::ios::app);
        if (!logFile_.is_open()) {
            std::cerr << "Warning: Could not open log file: " << logPath_ << std::endl;
        }
    }
}

void Logger::debug(const std::string& message) {
    log(LogLevel::DEBUG, message);
}

void Logger::info(const std::string& message) {
    log(LogLevel::INFO, message);
}

void Logger::warn(const std::string& message) {
    log(LogLevel::WARN, message);
}

void Logger::error(const std::string& message) {
    log(LogLevel::ERROR, message);
}

void Logger::log(LogLevel level, const std::string& message) {
    if (level < level_) {
        return;
    }
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::string timestamp = getCurrentTimestamp();
    std::string levelStr = levelToString(level);
    
    std::string fullMessage = "[" + timestamp + "] [" + levelStr + "] " + message;
    
    // Write to console
    if (level == LogLevel::ERROR) {
        std::cerr << fullMessage << std::endl;
    } else {
        std::cout << fullMessage << std::endl;
    }
    
    // Write to file
    if (logFile_.is_open()) {
        logFile_ << fullMessage << std::endl;
        logFile_.flush();
    }
}

std::string Logger::levelToString(LogLevel level) {
    switch (level) {
        case LogLevel::DEBUG: return "DEBUG";
        case LogLevel::INFO:  return "INFO";
        case LogLevel::WARN:  return "WARN";
        case LogLevel::ERROR: return "ERROR";
        default: return "UNKNOWN";
    }
}

std::string Logger::getCurrentTimestamp() {
    auto now = std::chrono::system_clock::now();
    auto time_t_now = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;
    
    std::stringstream ss;
    ss << std::put_time(std::localtime(&time_t_now), "%Y-%m-%d %H:%M:%S");
    ss << '.' << std::setfill('0') << std::setw(3) << ms.count();
    
    return ss.str();
}

} // namespace uitars
