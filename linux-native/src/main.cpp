/*
 * Copyright (c) 2025 Bytedance, Inc. and its affiliates.
 * SPDX-License-Identifier: Apache-2.0
 */
#include <iostream>
#include <string>
#include <cstring>
#include <getopt.h>
#include <signal.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>

#include "gui_agent.h"
#include "config.h"
#include "logger.h"
#include "service.h"

using namespace uitars;

static std::unique_ptr<Service> gService;
static std::unique_ptr<GUIAgent> gAgent;

void printVersion() {
    std::cout << "UI-TARS Linux Agent v0.2.4" << std::endl;
    std::cout << "Copyright (c) 2025 Bytedance, Inc." << std::endl;
}

void printHelp() {
    std::cout << "Usage: ui-tars-agent [OPTIONS]" << std::endl;
    std::cout << std::endl;
    std::cout << "A native Linux GUI agent for computer automation using Vision-Language Models." << std::endl;
    std::cout << std::endl;
    std::cout << "Options:" << std::endl;
    std::cout << "  -h, --help              Show this help message" << std::endl;
    std::cout << "  -v, --version           Show version information" << std::endl;
    std::cout << "  -c, --config <path>     Path to configuration file" << std::endl;
    std::cout << "  -i, --instruction <str> Run with a single instruction and exit" << std::endl;
    std::cout << "  -d, --daemon            Run as a system daemon/service" << std::endl;
    std::cout << "  -s, --socket <path>     Unix socket path for service mode" << std::endl;
    std::cout << "  -l, --log <path>        Log file path" << std::endl;
    std::cout << "  --provider <name>       VLM provider (openai, anthropic, volcengine)" << std::endl;
    std::cout << "  --model <name>          Model name" << std::endl;
    std::cout << "  --api-key <key>         API key for the VLM provider" << std::endl;
    std::cout << "  --base-url <url>        Base URL for the API endpoint" << std::endl;
    std::cout << std::endl;
    std::cout << "Service Control:" << std::endl;
    std::cout << "  ui-tars-agent --daemon         Start as daemon" << std::endl;
    std::cout << "  systemctl start ui-tars-agent  Start via systemd" << std::endl;
    std::cout << "  systemctl stop ui-tars-agent   Stop via systemd" << std::endl;
    std::cout << std::endl;
    std::cout << "Examples:" << std::endl;
    std::cout << "  # Run with an instruction" << std::endl;
    std::cout << "  ui-tars-agent -i \"Open Firefox and search for weather\"" << std::endl;
    std::cout << std::endl;
    std::cout << "  # Run with custom model provider" << std::endl;
    std::cout << "  ui-tars-agent --provider anthropic --model claude-3-5-sonnet --api-key sk-..." << std::endl;
    std::cout << std::endl;
    std::cout << "  # Start as daemon with custom socket" << std::endl;
    std::cout << "  ui-tars-agent -d -s /tmp/ui-tars.sock" << std::endl;
    std::cout << std::endl;
}

void signalHandler(int signum) {
    std::cout << "\nReceived signal " << signum << ", shutting down..." << std::endl;
    if (gService) {
        gService->stop();
    }
    if (gAgent) {
        gAgent->stop();
    }
}

void setupSignalHandlers() {
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);
    signal(SIGHUP, signalHandler);
}

int sendCommand(const std::string& socketPath, const std::string& command) {
    int sockFd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sockFd < 0) {
        std::cerr << "Error: Failed to create socket" << std::endl;
        return 1;
    }

    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, socketPath.c_str(), sizeof(addr.sun_path) - 1);

    if (connect(sockFd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        std::cerr << "Error: Failed to connect to service at " << socketPath << std::endl;
        close(sockFd);
        return 1;
    }

    std::string msg = command + "\n";
    if (write(sockFd, msg.c_str(), msg.size()) < 0) {
        std::cerr << "Error: Failed to send command" << std::endl;
        close(sockFd);
        return 1;
    }

    char buffer[4096];
    ssize_t n = read(sockFd, buffer, sizeof(buffer) - 1);
    if (n > 0) {
        buffer[n] = '\0';
        std::cout << buffer << std::endl;
    }

    close(sockFd);
    return 0;
}

int main(int argc, char* argv[]) {
    Config config = Config::loadFromEnv();
    std::string instruction;
    bool runAsDaemon = false;

    static struct option longOptions[] = {
        {"help",        no_argument,       nullptr, 'h'},
        {"version",     no_argument,       nullptr, 'v'},
        {"config",      required_argument, nullptr, 'c'},
        {"instruction", required_argument, nullptr, 'i'},
        {"daemon",      no_argument,       nullptr, 'd'},
        {"socket",      required_argument, nullptr, 's'},
        {"log",         required_argument, nullptr, 'l'},
        {"provider",    required_argument, nullptr, 'p'},
        {"model",       required_argument, nullptr, 'm'},
        {"api-key",     required_argument, nullptr, 'k'},
        {"base-url",    required_argument, nullptr, 'u'},
        {nullptr,       0,                 nullptr,  0 }
    };

    int opt;
    while ((opt = getopt_long(argc, argv, "hvc:i:ds:l:p:m:k:u:", longOptions, nullptr)) != -1) {
        switch (opt) {
            case 'h':
                printHelp();
                return 0;
            case 'v':
                printVersion();
                return 0;
            case 'c':
                config = Config::loadFromFile(optarg);
                break;
            case 'i':
                instruction = optarg;
                break;
            case 'd':
                runAsDaemon = true;
                config.runAsService = true;
                break;
            case 's':
                config.socketPath = optarg;
                break;
            case 'l':
                config.logPath = optarg;
                break;
            case 'p':
                config.model.provider = optarg;
                break;
            case 'm':
                config.model.model = optarg;
                break;
            case 'k':
                config.model.apiKey = optarg;
                break;
            case 'u':
                config.model.baseUrl = optarg;
                break;
            default:
                printHelp();
                return 1;
        }
    }

    // Validate configuration
    if (!config.validate()) {
        std::cerr << "Error: Invalid configuration. Please check your settings." << std::endl;
        std::cerr << "Required: --provider, --model, and --api-key" << std::endl;
        return 1;
    }

    // Setup logging
    auto logger = std::make_shared<Logger>(config.logPath);
    Logger::setInstance(logger);

    setupSignalHandlers();

    if (runAsDaemon) {
        // Run as daemon/service
        logger->info("Starting UI-TARS agent as daemon...");
        
        if (!Service::daemonize()) {
            logger->error("Failed to daemonize");
            return 1;
        }
        
        gService = std::make_unique<Service>(config);
        if (!gService->start()) {
            logger->error("Failed to start service");
            return 1;
        }
        
        // Wait for service to stop
        while (gService->isRunning()) {
            sleep(1);
        }
        
    } else if (!instruction.empty()) {
        // Run with single instruction
        logger->info("Running with instruction: " + instruction);
        
        gAgent = std::make_unique<GUIAgent>(config);
        
        gAgent->setDataCallback([&logger](const AgentData& data) {
            logger->info("Status: " + statusToString(data.status));
        });
        
        gAgent->setErrorCallback([&logger](const AgentError& error) {
            logger->error("Error: " + error.message);
        });
        
        if (!gAgent->run(instruction)) {
            logger->error("Agent execution failed");
            return 1;
        }
        
        logger->info("Agent execution completed");
        
    } else {
        // Interactive mode or send command to running service
        std::cout << "UI-TARS Linux Agent" << std::endl;
        std::cout << "Type 'help' for available commands or provide an instruction." << std::endl;
        std::cout << std::endl;
        
        gAgent = std::make_unique<GUIAgent>(config);
        
        gAgent->setDataCallback([](const AgentData& data) {
            std::cout << "[" << statusToString(data.status) << "] ";
            if (!data.conversations.empty()) {
                const auto& lastConv = data.conversations.back();
                if (lastConv.from == "gpt") {
                    std::cout << lastConv.value << std::endl;
                }
            }
        });
        
        gAgent->setErrorCallback([](const AgentError& error) {
            std::cerr << "Error: " << error.message << std::endl;
        });
        
        std::string input;
        while (std::cout << "> " && std::getline(std::cin, input)) {
            if (input.empty()) continue;
            
            if (input == "quit" || input == "exit") {
                break;
            } else if (input == "help") {
                std::cout << "Commands:" << std::endl;
                std::cout << "  <instruction>  Execute an instruction" << std::endl;
                std::cout << "  pause          Pause current execution" << std::endl;
                std::cout << "  resume         Resume paused execution" << std::endl;
                std::cout << "  stop           Stop current execution" << std::endl;
                std::cout << "  quit/exit      Exit the program" << std::endl;
            } else if (input == "pause") {
                gAgent->pause();
                std::cout << "Paused" << std::endl;
            } else if (input == "resume") {
                gAgent->resume();
                std::cout << "Resumed" << std::endl;
            } else if (input == "stop") {
                gAgent->stop();
                std::cout << "Stopped" << std::endl;
            } else {
                gAgent->run(input);
            }
        }
    }

    return 0;
}
