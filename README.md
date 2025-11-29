# UI-TARS Linux Agent

<picture>
  <img alt="UI-TARS Linux" src="./images/tars.png">
</picture>

<br/>

## Introduction

🐧 **UI-TARS Linux** is a native Linux GUI agent for computer automation using Vision-Language Models. This fork focuses exclusively on Linux support with a native C++ implementation.

<p>
    <a href="https://discord.gg/HnKcSBgTVx"><img src="https://img.shields.io/badge/Discord-Join%20Community-5865F2?style=for-the-badge&logo=discord&logoColor=white" alt="Discord Community" /></a>
    <a href="https://github.com/bytedance/UI-TARS-desktop"><img src="https://img.shields.io/badge/Upstream-ByteDance-EF4444?style=for-the-badge&logo=github&logoColor=white" alt="Upstream Repo" /></a>
</p>

## Features

- 🐧 **Native Linux Support** - Built specifically for Linux with X11 support
- 🚀 **High Performance C++** - Native C++ implementation for optimal performance
- 📦 **Easy Installation** - Available as .deb package or single static binary
- 🔄 **Systemd Service** - Run as a Linux background service
- 🎯 **GUI Automation** - Full mouse, keyboard, and screen capture control
- 🤖 **VLM Integration** - Supports OpenAI, Anthropic, and Volcengine models

## Table of Contents

- [Quick Start](#quick-start)
- [Installation](#installation)
  - [From Debian Package](#from-debian-package)
  - [Single Binary](#single-binary)
  - [Build from Source](#build-from-source)
- [Configuration](#configuration)
- [Usage](#usage)
  - [Command Line](#command-line)
  - [Systemd Service](#systemd-service)
- [Supported Providers](#supported-providers)
- [Troubleshooting](#troubleshooting)
- [License](#license)

## Quick Start

### Install Dependencies (Ubuntu/Debian)

```bash
sudo apt install libx11-6 libxtst6 libcurl4 libpng16-16
```

### Install from .deb Package

```bash
# Download the latest release
wget https://github.com/ramgeart/UI-TARS-desktop/releases/latest/download/ui-tars-agent.deb

# Install
sudo dpkg -i ui-tars-agent.deb
```

### Configure and Run

```bash
# Set your API key
export UI_TARS_API_KEY=your-api-key-here

# Run with an instruction
ui-tars-agent --provider openai --model gpt-4-vision-preview \
              -i "Open Firefox and search for weather"
```

## Installation

### From Debian Package

The easiest way to install is using the pre-built .deb package:

```bash
# Install the package
sudo dpkg -i ui-tars-agent-0.2.4.deb

# Install any missing dependencies
sudo apt -f install
```

This will install:
- The `ui-tars-agent` binary to `/usr/bin/`
- Configuration files to `/etc/ui-tars/`
- Systemd service file to `/lib/systemd/system/`

### Single Binary

For portable use, download the static binary:

```bash
# Download static binary
wget https://github.com/ramgeart/UI-TARS-desktop/releases/latest/download/ui-tars-agent-static

# Make executable
chmod +x ui-tars-agent-static

# Run directly
./ui-tars-agent-static --help
```

### Build from Source

#### Install Build Dependencies

```bash
sudo apt install build-essential cmake pkg-config \
                 libx11-dev libxtst-dev libxinerama-dev \
                 libcurl4-openssl-dev libpng-dev libjpeg-dev
```

#### Build

```bash
cd linux-native

# Standard build with Debian package
./scripts/build.sh

# Static binary (single executable)
./scripts/build.sh --static

# Install after building
sudo ./scripts/install.sh
```

## Configuration

### Configuration File

Edit `/etc/ui-tars/ui-tars-agent.conf`:

```ini
# Model Configuration
provider = openai
model = gpt-4-vision-preview
# api_key = your-api-key  # Or use environment variable

# Agent Configuration
max_loop_count = 50
loop_interval_ms = 1000

# Service Configuration
socket_path = /var/run/ui-tars-agent.sock
log_path = /var/log/ui-tars-agent.log
```

### Environment Variables

```bash
# Required
export UI_TARS_API_KEY=your-api-key-here

# Optional overrides
export UI_TARS_PROVIDER=openai
export UI_TARS_MODEL=gpt-4-vision-preview
export UI_TARS_MAX_LOOP_COUNT=50
```

### API Key File (for systemd service)

Edit `/etc/ui-tars/ui-tars-agent.env`:

```bash
UI_TARS_API_KEY=your-api-key-here
```

**Important:** Set secure permissions on this file:
```bash
sudo chmod 600 /etc/ui-tars/ui-tars-agent.env
```

## Usage

### Command Line

```bash
# Show help
ui-tars-agent --help

# Run with a single instruction
ui-tars-agent -i "Open the terminal and run ls -la"

# Run with custom provider
ui-tars-agent --provider anthropic \
              --model claude-3-5-sonnet \
              --api-key sk-your-key \
              -i "Take a screenshot and describe what you see"

# Interactive mode
ui-tars-agent
> Open Firefox
> Navigate to google.com
> Search for "weather forecast"
> quit
```

### Systemd Service

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

#### Sending Commands to the Service

```bash
# Run an instruction
echo "run Open the file manager" | nc -U /var/run/ui-tars-agent.sock

# Check status
echo "status" | nc -U /var/run/ui-tars-agent.sock

# Pause execution
echo "pause" | nc -U /var/run/ui-tars-agent.sock

# Resume execution
echo "resume" | nc -U /var/run/ui-tars-agent.sock

# Stop execution
echo "stop" | nc -U /var/run/ui-tars-agent.sock
```

## Supported Providers

| Provider | Example Models | Configuration |
|----------|----------------|---------------|
| OpenAI | `gpt-4-vision-preview`, `gpt-4o` | `--provider openai` |
| Anthropic | `claude-3-5-sonnet`, `claude-3-opus` | `--provider anthropic` |
| Volcengine | `doubao-1-5-thinking-vision-pro` | `--provider volcengine` |
| Custom | Any OpenAI-compatible | `--provider custom --base-url https://...` |

### Custom Provider Example

```bash
ui-tars-agent --provider custom \
              --base-url https://your-api.com/v1/chat/completions \
              --model your-model-name \
              --api-key your-key \
              -i "Your instruction"
```

## System Requirements

- **OS**: Linux (Ubuntu 20.04+, Debian 11+, or compatible)
- **Display**: X11 (Wayland support planned)
- **RAM**: At least 2GB
- **Network**: Internet access for VLM API calls

### Runtime Dependencies

```bash
# Ubuntu/Debian
sudo apt install libx11-6 libxtst6 libxinerama1 libcurl4 libpng16-16
```

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

For the systemd service, edit the service file:
```bash
sudo systemctl edit ui-tars-agent

# Add:
[Service]
Environment=DISPLAY=:0
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

# Clear build cache
rm -rf linux-native/build
cd linux-native && ./scripts/build.sh --clean
```

## Project Structure

```
linux-native/
├── include/           # C++ header files
├── src/               # C++ implementation
├── config/            # Default configuration files
├── systemd/           # Systemd service files
├── debian/            # Debian package scripts
├── scripts/           # Build and install scripts
└── CMakeLists.txt     # CMake build configuration
```

For detailed documentation, see [linux-native/README.md](./linux-native/README.md).

## Contributing

See [CONTRIBUTING.md](./CONTRIBUTING.md).

## License

This project is licensed under the Apache License 2.0.

## Citation

If you find our paper and code useful in your research, please consider giving a star ⭐ and citation 📝

```BibTeX
@article{qin2025ui,
  title={UI-TARS: Pioneering Automated GUI Interaction with Native Agents},
  author={Qin, Yujia and Ye, Yining and Fang, Junjie and Wang, Haoming and Liang, Shihao and Tian, Shizuo and Zhang, Junda and Li, Jiahao and Li, Yunxin and Huang, Shijue and others},
  journal={arXiv preprint arXiv:2501.12326},
  year={2025}
}
```

## Acknowledgments

This project is a Linux-focused fork of [UI-TARS-desktop](https://github.com/bytedance/UI-TARS-desktop) by ByteDance.
