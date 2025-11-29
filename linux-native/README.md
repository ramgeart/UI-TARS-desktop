# UI-TARS Linux Native Agent

A native Linux C++ implementation of the UI-TARS GUI Agent for computer automation using Vision-Language Models.

## Features

- 🐧 **Native Linux Support** - Built specifically for Linux with X11 support
- 🚀 **High Performance** - C++ implementation for optimal performance
- 📦 **Easy Installation** - Available as .deb package or single binary
- 🔄 **Service Mode** - Run as a systemd service
- 🎯 **GUI Automation** - Mouse, keyboard, and screen capture control
- 🤖 **VLM Integration** - Supports OpenAI, Anthropic, and Volcengine models

## System Requirements

- Linux (Ubuntu 20.04+, Debian 11+, or compatible)
- X11 display server (Wayland support planned)
- At least 2GB RAM
- Network access for VLM API calls

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

# Clean build
./scripts/build.sh --clean
```

### Manual CMake Build

```bash
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
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
provider = openai
model = gpt-4-vision-preview
temperature = 0.0
max_tokens = 4096

# Agent Configuration
max_loop_count = 50
loop_interval_ms = 1000

# Service Configuration
socket_path = /var/run/ui-tars-agent.sock
log_path = /var/log/ui-tars-agent.log
```

### Environment Variables

Set your API key in `/etc/ui-tars/ui-tars-agent.env`:

```bash
UI_TARS_API_KEY=your-api-key-here
```

Or export directly:

```bash
export UI_TARS_API_KEY=your-api-key-here
```

## Usage

### Command Line

```bash
# Show help
ui-tars-agent --help

# Run with an instruction
ui-tars-agent -i "Open Firefox and search for weather"

# Run with custom provider
ui-tars-agent --provider anthropic \
              --model claude-3-5-sonnet \
              --api-key sk-your-key \
              -i "Take a screenshot"

# Interactive mode
ui-tars-agent
> Open the terminal
> Type 'hello world'
> quit
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

## Supported VLM Providers

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
