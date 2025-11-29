# UI-TARS Linux Native Agent

A native Linux C++ implementation of the UI-TARS GUI Agent for computer automation using Vision-Language Models. Supports both API-based and **local embedded inference** with llama.cpp.

## Features

- 🐧 **Native Linux Support** - Built specifically for Linux with X11 support
- 🧠 **Local Inference** - Run UI-TARS-1.5-7B models locally with llama.cpp (no API key needed!)
- 🚀 **High Performance** - C++ implementation for optimal performance
- 📦 **Easy Installation** - Available as .deb package or single binary
- 🔄 **Service Mode** - Run as a systemd service
- 💬 **Chat Interface** - Interactive chat mode with command support
- 🎯 **GUI Automation** - Mouse, keyboard, and screen capture control
- 🤖 **VLM Integration** - Supports OpenAI, Anthropic, Volcengine, and local GGUF models

## System Requirements

- Linux (Ubuntu 20.04+, Debian 11+, or compatible)
- X11 display server (Wayland support planned)
- For local inference:
  - At least 8GB RAM (16GB recommended for 7B models)
  - CUDA GPU optional but recommended
- For API mode:
  - At least 2GB RAM
  - Network access for VLM API calls

## Quick Start - Local Inference (Recommended)

Run UI-TARS completely offline without any API keys!

### 1. Download GGUF Models

```bash
# Create models directory
mkdir -p ~/models

# Download UI-TARS-1.5-7B quantized model (Q4_K_S - ~4GB)
wget -O ~/models/UI-TARS-1.5-7B.gguf \
  https://huggingface.co/mradermacher/UI-TARS-1.5-7B-i1-GGUF/resolve/main/UI-TARS-1.5-7B.i1-Q4_K_S.gguf

# Download multimodal projector (for vision support)
wget -O ~/models/UI-TARS-1.5-7B.mmproj.gguf \
  https://huggingface.co/mradermacher/UI-TARS-1.5-7B-GGUF/resolve/main/UI-TARS-1.5-7B.mmproj-Q8_0.gguf
```

### 2. Run with Local Model

```bash
# Run with single instruction
ui-tars-agent --provider local \
              --model-path ~/models/UI-TARS-1.5-7B.gguf \
              --mmproj-path ~/models/UI-TARS-1.5-7B.mmproj.gguf \
              -i "Open Firefox and search for weather"

# Run in interactive chat mode
ui-tars-agent --provider local \
              --model-path ~/models/UI-TARS-1.5-7B.gguf \
              --mmproj-path ~/models/UI-TARS-1.5-7B.mmproj.gguf
```

## Dependencies

```bash
# Ubuntu/Debian
sudo apt install libx11-dev libxtst-dev libxinerama-dev \
                 libcurl4-openssl-dev libpng-dev libjpeg-dev \
                 cmake g++ pkg-config
```

## Building from Source

### Quick Build

```bash
cd linux-native
./scripts/build.sh
```

### Build Options

```bash
# Standard build with Debian package
./scripts/build.sh

# Static binary (single executable)
./scripts/build.sh --static

# With CUDA GPU support
./scripts/build.sh --cuda

# Clean build
./scripts/build.sh --clean
```

### Manual CMake Build

```bash
mkdir build && cd build

# Standard build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)

# With CUDA support for faster local inference
cmake .. -DCMAKE_BUILD_TYPE=Release -DLLAMA_CUBLAS=ON
make -j$(nproc)

# Create Debian package
cpack -G DEB
```

## Installation

### From Debian Package

```bash
# Install the .deb package
sudo dpkg -i ui-tars-agent-0.2.4.deb

# Install dependencies if needed
sudo apt -f install
```

### Manual Installation

```bash
# After building
sudo ./scripts/install.sh
```

### Single Binary

For portable use, copy the static binary:

```bash
# Build static binary
./scripts/build.sh --static --no-deb

# Copy to desired location
cp build/ui-tars-agent /path/to/desired/location/
```

## Configuration

### Configuration File

Edit `/etc/ui-tars/ui-tars-agent.conf`:

```ini
# Model Configuration
# Use provider = local for embedded inference
provider = local

# For API providers (openai, anthropic, volcengine):
# provider = openai
# model = gpt-4-vision-preview
# api_key = your-api-key

# Local Model Configuration
model_path = /home/user/models/UI-TARS-1.5-7B.gguf
mmproj_path = /home/user/models/UI-TARS-1.5-7B.mmproj.gguf
n_ctx = 4096
n_batch = 512
n_threads = 4
n_gpu_layers = 0  # Set to 35 for full GPU offload

# Agent Configuration
max_loop_count = 50
loop_interval_ms = 1000

# Service Configuration
socket_path = /var/run/ui-tars-agent.sock
log_path = /var/log/ui-tars-agent.log
```

### Environment Variables

For API providers, set in `/etc/ui-tars/ui-tars-agent.env`:

```bash
UI_TARS_API_KEY=your-api-key-here
```

For local models:

```bash
export UI_TARS_PROVIDER=local
export UI_TARS_MODEL_PATH=/path/to/model.gguf
export UI_TARS_MMPROJ_PATH=/path/to/mmproj.gguf
export UI_TARS_N_GPU_LAYERS=35  # For GPU acceleration
```

## Usage

### Interactive Chat Mode

```bash
# Start chat interface with local model
ui-tars-agent --provider local --model-path ~/models/UI-TARS-1.5-7B.gguf

# ╔══════════════════════════════════════════════════════════════╗
# ║           UI-TARS Linux Agent - Chat Interface               ║
# ╚══════════════════════════════════════════════════════════════╝
# Using: Local model (UI-TARS-1.5-7B)
# 
# === UI-TARS Chat Interface ===
# Commands:
#   /help     - Show this help
#   /run      - Execute the last instruction
#   /pause    - Pause current execution
#   /resume   - Resume paused execution
#   /stop     - Stop current execution
#   /status   - Show agent status
#   /clear    - Clear conversation history
#   /quit     - Exit the program
# 
# > Open Firefox and search for "weather"
# Executing: Open Firefox and search for "weather"
# [running] Taking screenshot...
# [running] Action: click(element='Firefox icon')
# ...
```

### Command Line

```bash
# Show help
ui-tars-agent --help

# Run with local model
ui-tars-agent --provider local \
              --model-path ~/models/UI-TARS-1.5-7B.gguf \
              -i "Open Firefox and search for weather"

# Run with API provider
ui-tars-agent --provider anthropic \
              --model claude-3-5-sonnet \
              --api-key sk-your-key \
              -i "Take a screenshot"

# Local model with GPU acceleration
ui-tars-agent --provider local \
              --model-path ~/models/UI-TARS-1.5-7B.gguf \
              --n-gpu-layers 35 \
              -i "Open terminal and run ls"
```

### As a Systemd Service

```bash
# Start the service
sudo systemctl start ui-tars-agent

# Enable at boot
sudo systemctl enable ui-tars-agent

# Check status
sudo systemctl status ui-tars-agent

# View logs
journalctl -u ui-tars-agent -f
```

### Client Commands (when running as service)

```bash
# Send instruction to running service
echo "run Open Firefox" | nc -U /var/run/ui-tars-agent.sock

# Check status
echo "status" | nc -U /var/run/ui-tars-agent.sock

# Pause execution
echo "pause" | nc -U /var/run/ui-tars-agent.sock

# Resume execution
echo "resume" | nc -U /var/run/ui-tars-agent.sock

# Stop execution
echo "stop" | nc -U /var/run/ui-tars-agent.sock
```

## Supported Models

### Local GGUF Models (Embedded Inference)

| Model | Size | VRAM | Notes |
|-------|------|------|-------|
| UI-TARS-1.5-7B.i1-Q4_K_S.gguf | ~4GB | 6GB | Recommended, good balance |
| UI-TARS-1.5-7B.i1-Q5_K_M.gguf | ~5GB | 8GB | Higher quality |
| UI-TARS-1.5-7B.i1-Q8_0.gguf | ~7GB | 10GB | Highest quality |

Download from: https://huggingface.co/mradermacher/UI-TARS-1.5-7B-i1-GGUF

### API Providers

| Provider | Models | Notes |
|----------|--------|-------|
| OpenAI | gpt-4-vision-preview, gpt-4o | Standard OpenAI API |
| Anthropic | claude-3-5-sonnet, claude-3-opus | Requires x-api-key header |
| Volcengine | doubao-1-5-thinking-vision-pro | Chinese provider |

### Custom Providers

For custom OpenAI-compatible endpoints:

```bash
ui-tars-agent --provider custom \
              --base-url https://your-api.com/v1/chat/completions \
              --model your-model-name \
              --api-key your-key
```

## Performance Tips

### For Local Inference

1. **Use GPU Offloading** - Set `--n-gpu-layers 35` for full GPU offload (requires CUDA build)
2. **Adjust Context Size** - Lower `--n-ctx 2048` for faster inference
3. **Thread Count** - Match `--n-threads` to your CPU cores
4. **Use Quantized Models** - Q4_K_S offers best speed/quality ratio

### Memory Requirements

| Model Quantization | RAM (CPU) | VRAM (GPU) |
|-------------------|-----------|------------|
| Q4_K_S | 8GB | 6GB |
| Q5_K_M | 10GB | 8GB |
| Q8_0 | 16GB | 10GB |

## Troubleshooting

### X11 Display Issues

If running as a service, ensure the display is configured:

```bash
# Check if X11 is accessible
export DISPLAY=:0
xdpyinfo

# Grant X11 access (if needed)
xhost +local:
```

### Local Model Loading Issues

```bash
# Check if model file exists
ls -la ~/models/UI-TARS-1.5-7B.gguf

# Verify file integrity
sha256sum ~/models/UI-TARS-1.5-7B.gguf

# Run with debug logging
ui-tars-agent --provider local --model-path ~/models/UI-TARS-1.5-7B.gguf -l /tmp/debug.log
```

### Permission Issues

For screenshot and input control:

```bash
# Add user to input group
sudo usermod -a -G input $USER

# Restart or re-login to apply
```

### Build Issues

```bash
# Install all build dependencies
sudo apt install build-essential cmake pkg-config \
                 libx11-dev libxtst-dev libxinerama-dev \
                 libcurl4-openssl-dev libpng-dev libjpeg-dev

# Clear CMake cache
rm -rf build && ./scripts/build.sh --clean
```

## Architecture

```
linux-native/
├── include/           # Header files
│   ├── gui_agent.h    # Main agent class
│   ├── llama_inference.h  # Local inference with llama.cpp
│   ├── screenshot.h   # Screen capture
│   ├── input_controller.h  # Mouse/keyboard control
│   ├── vlm_client.h   # VLM API client
│   ├── action_parser.h    # Parse VLM responses
│   ├── config.h       # Configuration
│   ├── logger.h       # Logging
│   ├── service.h      # Daemon/service
│   └── types.h        # Type definitions
├── src/               # Implementation files
├── config/            # Default configuration
├── systemd/           # Systemd service files
├── debian/            # Debian package scripts
├── scripts/           # Build and install scripts
└── CMakeLists.txt     # Build configuration
```

## License

Apache License 2.0

## Contributing

See [CONTRIBUTING.md](../CONTRIBUTING.md) for guidelines.
