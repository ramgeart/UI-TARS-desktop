#!/bin/bash
# Installation script for UI-TARS Linux Agent
# Usage: sudo ./install.sh

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(dirname "${SCRIPT_DIR}")"
BUILD_DIR="${PROJECT_DIR}/build"

# Check if running as root
if [ "$EUID" -ne 0 ]; then
    echo "Please run as root: sudo ./install.sh"
    exit 1
fi

# Check if binary exists
if [ ! -f "${BUILD_DIR}/ui-tars-agent" ]; then
    echo "Binary not found. Please build first: ./scripts/build.sh"
    exit 1
fi

echo "Installing UI-TARS Linux Agent..."

# Install binary
echo "Installing binary..."
cp "${BUILD_DIR}/ui-tars-agent" /usr/bin/
chmod +x /usr/bin/ui-tars-agent

# Create directories
echo "Creating directories..."
mkdir -p /etc/ui-tars
mkdir -p /usr/share/ui-tars

# Install configuration files
echo "Installing configuration files..."
cp "${PROJECT_DIR}/config/ui-tars-agent.conf" /usr/share/ui-tars/
cp "${PROJECT_DIR}/config/ui-tars-agent.env" /usr/share/ui-tars/

if [ ! -f /etc/ui-tars/ui-tars-agent.conf ]; then
    cp "${PROJECT_DIR}/config/ui-tars-agent.conf" /etc/ui-tars/
fi

if [ ! -f /etc/ui-tars/ui-tars-agent.env ]; then
    cp "${PROJECT_DIR}/config/ui-tars-agent.env" /etc/ui-tars/
    chmod 600 /etc/ui-tars/ui-tars-agent.env
fi

# Install systemd service
echo "Installing systemd service..."
cp "${PROJECT_DIR}/systemd/ui-tars-agent.service" /lib/systemd/system/
systemctl daemon-reload

echo ""
echo "Installation complete!"
echo ""
echo "Next steps:"
echo "  1. Edit /etc/ui-tars/ui-tars-agent.conf with your settings"
echo "  2. Add your API key to /etc/ui-tars/ui-tars-agent.env"
echo "  3. Start the service: sudo systemctl start ui-tars-agent"
echo "  4. Enable at boot: sudo systemctl enable ui-tars-agent"
echo ""
echo "Or run directly: ui-tars-agent --help"
