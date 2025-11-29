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
#include "llama_inference.h"

using namespace uitars;

static std::unique_ptr<Service> gService;
static std::unique_ptr<GUIAgent> gAgent;
static std::unique_ptr<LlamaInference> gLocalModel;

void printVersion() {
    std::cout << "UI-TARS Linux Agent v0.2.4" << std::endl;
    std::cout << "Copyright (c) 2025 Bytedance, Inc." << std::endl;
    std::cout << "With embedded inference support (llama.cpp)" << std::endl;
}

void printHelp() {
    std::cout << "Usage: ui-tars-agent [OPTIONS]" << std::endl;
    std::cout << std::endl;
    std::cout << "A native Linux GUI agent for computer automation using Vision-Language Models." << std::endl;
    std::cout << "Supports both API-based and local embedded inference with GGUF models." << std::endl;
    std::cout << std::endl;
    std::cout << "Options:" << std::endl;
    std::cout << "  -h, --help              Show this help message" << std::endl;
    std::cout << "  -v, --version           Show version information" << std::endl;
    std::cout << "  -c, --config <path>     Path to configuration file" << std::endl;
    std::cout << "  -i, --instruction <str> Run with a single instruction and exit" << std::endl;
    std::cout << "  -d, --daemon            Run as a system daemon/service" << std::endl;
    std::cout << "  -s, --socket <path>     Unix socket path for service mode" << std::endl;
    std::cout << "  -l, --log <path>        Log file path" << std::endl;
    std::cout << std::endl;
    std::cout << "API Provider Options:" << std::endl;
    std::cout << "  --provider <name>       VLM provider (openai, anthropic, volcengine, local)" << std::endl;
    std::cout << "  --model <name>          Model name" << std::endl;
    std::cout << "  --api-key <key>         API key for the VLM provider" << std::endl;
    std::cout << "  --base-url <url>        Base URL for the API endpoint" << std::endl;
    std::cout << std::endl;
    std::cout << "Local Inference Options (provider=local):" << std::endl;
    std::cout << "  --model-path <path>     Path to GGUF model file" << std::endl;
    std::cout << "  --mmproj-path <path>    Path to multimodal projector GGUF" << std::endl;
    std::cout << "  --n-ctx <size>          Context size (default: 4096)" << std::endl;
    std::cout << "  --n-threads <num>       Number of threads (default: 4)" << std::endl;
    std::cout << "  --n-gpu-layers <num>    GPU layers to offload (default: 0)" << std::endl;
    std::cout << std::endl;
    std::cout << "Recommended GGUF Models:" << std::endl;
    std::cout << "  Main model:    UI-TARS-1.5-7B.i1-Q4_K_S.gguf" << std::endl;
    std::cout << "  MM Projector:  UI-TARS-1.5-7B.mmproj-Q8_0.gguf" << std::endl;
    std::cout << "  Download from: https://huggingface.co/mradermacher/UI-TARS-1.5-7B-i1-GGUF" << std::endl;
    std::cout << std::endl;
    std::cout << "Service Control:" << std::endl;
    std::cout << "  ui-tars-agent --daemon         Start as daemon" << std::endl;
    std::cout << "  systemctl start ui-tars-agent  Start via systemd" << std::endl;
    std::cout << "  systemctl stop ui-tars-agent   Stop via systemd" << std::endl;
    std::cout << std::endl;
    std::cout << "Examples:" << std::endl;
    std::cout << "  # Run with local model (no API key needed)" << std::endl;
    std::cout << "  ui-tars-agent --provider local --model-path ~/models/UI-TARS-1.5-7B.gguf \\" << std::endl;
    std::cout << "                --mmproj-path ~/models/UI-TARS-1.5-7B.mmproj-Q8_0.gguf \\" << std::endl;
    std::cout << "                -i \"Open Firefox and search for weather\"" << std::endl;
    std::cout << std::endl;
    std::cout << "  # Run with API provider" << std::endl;
    std::cout << "  ui-tars-agent --provider openai --model gpt-4-vision-preview --api-key sk-..." << std::endl;
    std::cout << std::endl;
    std::cout << "  # Interactive chat mode" << std::endl;
    std::cout << "  ui-tars-agent --provider local --model-path ~/models/UI-TARS-1.5-7B.gguf" << std::endl;
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
    if (gLocalModel) {
        gLocalModel->unloadModel();
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

void printChatHelp() {
    std::cout << "\n=== UI-TARS Chat Interface ===" << std::endl;
    std::cout << "Commands:" << std::endl;
    std::cout << "  /help     - Show this help" << std::endl;
    std::cout << "  /run      - Execute the last instruction" << std::endl;
    std::cout << "  /pause    - Pause current execution" << std::endl;
    std::cout << "  /resume   - Resume paused execution" << std::endl;
    std::cout << "  /stop     - Stop current execution" << std::endl;
    std::cout << "  /status   - Show agent status" << std::endl;
    std::cout << "  /clear    - Clear conversation history" << std::endl;
    std::cout << "  /quit     - Exit the program" << std::endl;
    std::cout << "\nType any instruction to have the agent execute it." << std::endl;
    std::cout << "==============================\n" << std::endl;
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
        // Local model options
        {"model-path",  required_argument, nullptr, 'M'},
        {"mmproj-path", required_argument, nullptr, 'P'},
        {"n-ctx",       required_argument, nullptr, 'C'},
        {"n-threads",   required_argument, nullptr, 'T'},
        {"n-gpu-layers", required_argument, nullptr, 'G'},
        {nullptr,       0,                 nullptr,  0 }
    };

    int opt;
    while ((opt = getopt_long(argc, argv, "hvc:i:ds:l:p:m:k:u:M:P:C:T:G:", longOptions, nullptr)) != -1) {
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
            case 'M':
                config.localModel.modelPath = optarg;
                config.model.provider = "local";
                break;
            case 'P':
                config.localModel.mmprojPath = optarg;
                break;
            case 'C':
                config.localModel.nCtx = std::stoi(optarg);
                break;
            case 'T':
                config.localModel.nThreads = std::stoi(optarg);
                break;
            case 'G':
                config.localModel.nGpuLayers = std::stoi(optarg);
                break;
            default:
                printHelp();
                return 1;
        }
    }

    // Validate configuration
    if (!config.validate()) {
        std::cerr << "Error: Invalid configuration. Please check your settings." << std::endl;
        if (config.model.provider == "local") {
            std::cerr << "For local inference: --model-path is required" << std::endl;
        } else {
            std::cerr << "For API providers: --provider, --model, and --api-key are required" << std::endl;
        }
        return 1;
    }

    // Setup logging
    auto logger = std::make_shared<Logger>(config.logPath);
    Logger::setInstance(logger);

    setupSignalHandlers();

    // Load local model if using local provider
    if (config.isLocalModel()) {
        logger->info("Using local inference with llama.cpp");
        gLocalModel = std::make_unique<LlamaInference>(config.localModel, logger);
        
        std::cout << "Loading model: " << config.localModel.modelPath << std::endl;
        if (!gLocalModel->loadModel()) {
            std::cerr << "Error: Failed to load local model" << std::endl;
            return 1;
        }
        std::cout << "Model loaded successfully: " << gLocalModel->getModelName() << std::endl;
        std::cout << "Model size: " << (gLocalModel->getModelSize() / 1024 / 1024) << " MB" << std::endl;
    }

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
        // Interactive chat mode
        std::cout << std::endl;
        std::cout << "╔══════════════════════════════════════════════════════════════╗" << std::endl;
        std::cout << "║           UI-TARS Linux Agent - Chat Interface               ║" << std::endl;
        std::cout << "╚══════════════════════════════════════════════════════════════╝" << std::endl;
        
        if (config.isLocalModel()) {
            std::cout << "Using: Local model (" << gLocalModel->getModelName() << ")" << std::endl;
        } else {
            std::cout << "Using: " << config.model.provider << "/" << config.model.model << std::endl;
        }
        
        printChatHelp();
        
        gAgent = std::make_unique<GUIAgent>(config);
        
        gAgent->setDataCallback([](const AgentData& data) {
            std::cout << "[" << statusToString(data.status) << "] ";
            if (!data.conversations.empty()) {
                const auto& lastConv = data.conversations.back();
                if (lastConv.from == "gpt") {
                    std::cout << "\n" << lastConv.value << std::endl;
                }
            }
        });
        
        gAgent->setErrorCallback([](const AgentError& error) {
            std::cerr << "\n[ERROR] " << error.message << std::endl;
        });
        
        std::string input;
        std::string lastInstruction;
        
        while (true) {
            std::cout << "\n> ";
            if (!std::getline(std::cin, input)) {
                break;
            }
            
            if (input.empty()) continue;
            
            // Handle commands
            if (input[0] == '/') {
                if (input == "/quit" || input == "/exit" || input == "/q") {
                    std::cout << "Goodbye!" << std::endl;
                    break;
                } else if (input == "/help" || input == "/h") {
                    printChatHelp();
                } else if (input == "/run") {
                    if (lastInstruction.empty()) {
                        std::cout << "No previous instruction to run." << std::endl;
                    } else {
                        std::cout << "Re-running: " << lastInstruction << std::endl;
                        gAgent->run(lastInstruction);
                    }
                } else if (input == "/pause") {
                    gAgent->pause();
                    std::cout << "Paused" << std::endl;
                } else if (input == "/resume") {
                    gAgent->resume();
                    std::cout << "Resumed" << std::endl;
                } else if (input == "/stop") {
                    gAgent->stop();
                    std::cout << "Stopped" << std::endl;
                } else if (input == "/status") {
                    if (gAgent->isRunning()) {
                        std::cout << "Agent is " << (gAgent->isPaused() ? "paused" : "running") << std::endl;
                    } else {
                        std::cout << "Agent is idle" << std::endl;
                    }
                } else if (input == "/clear") {
                    // Reset agent
                    gAgent = std::make_unique<GUIAgent>(config);
                    std::cout << "Conversation cleared" << std::endl;
                } else {
                    std::cout << "Unknown command: " << input << std::endl;
                    std::cout << "Type /help for available commands." << std::endl;
                }
            } else {
                // Execute instruction
                lastInstruction = input;
                std::cout << "\nExecuting: " << input << std::endl;
                gAgent->run(input);
            }
        }
    }

    // Cleanup
    if (gLocalModel) {
        gLocalModel->unloadModel();
    }

    return 0;
}
