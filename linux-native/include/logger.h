/*
 * Copyright (c) 2025 Bytedance, Inc. and its affiliates.
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef UI_TARS_LOGGER_H
#define UI_TARS_LOGGER_H

#include <string>
#include <fstream>
#include <mutex>
#include <memory>

namespace uitars {

enum class LogLevel {
    DEBUG,
    INFO,
    WARN,
    ERROR
};

class Logger {
public:
    Logger(const std::string& logPath = "", LogLevel level = LogLevel::INFO);
    ~Logger();

    void debug(const std::string& message);
    void info(const std::string& message);
    void warn(const std::string& message);
    void error(const std::string& message);

    void setLevel(LogLevel level);
    void setLogPath(const std::string& path);

    static std::shared_ptr<Logger> getInstance();
    static void setInstance(std::shared_ptr<Logger> logger);

private:
    std::string logPath_;
    LogLevel level_;
    std::ofstream logFile_;
    std::mutex mutex_;
    
    static std::shared_ptr<Logger> instance_;
    static std::mutex instanceMutex_;

    void log(LogLevel level, const std::string& message);
    std::string levelToString(LogLevel level);
    std::string getCurrentTimestamp();
};

} // namespace uitars

#endif // UI_TARS_LOGGER_H
