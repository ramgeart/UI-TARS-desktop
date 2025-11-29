#!/bin/bash
# Build script for UI-TARS Linux Agent
# Usage: ./build.sh [--static] [--deb] [--clean]

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="${SCRIPT_DIR}/build"
BUILD_STATIC=OFF
BUILD_DEB=ON

# Parse arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        --static)
            BUILD_STATIC=ON
            shift
            ;;
        --deb)
            BUILD_DEB=ON
            shift
            ;;
        --no-deb)
            BUILD_DEB=OFF
            shift
            ;;
        --clean)
            echo "Cleaning build directory..."
            rm -rf "${BUILD_DIR}"
            shift
            ;;
        --help)
            echo "Usage: ./build.sh [OPTIONS]"
            echo ""
            echo "Options:"
            echo "  --static   Build static binary (single executable)"
            echo "  --deb      Build Debian package (default: yes)"
            echo "  --no-deb   Don't build Debian package"
            echo "  --clean    Clean build directory before building"
            echo "  --help     Show this help message"
            exit 0
            ;;
        *)
            echo "Unknown option: $1"
            exit 1
            ;;
    esac
done

# Check dependencies
echo "Checking dependencies..."

check_dependency() {
    if ! command -v "$1" &> /dev/null; then
        echo "Error: $1 is not installed"
        echo "Please install it with: sudo apt install $2"
        exit 1
    fi
}

check_pkg_config() {
    if ! pkg-config --exists "$1" 2>/dev/null; then
        echo "Error: $1 development files not found"
        echo "Please install with: sudo apt install $2"
        exit 1
    fi
}

check_dependency cmake cmake
check_dependency pkg-config pkg-config
check_dependency g++ g++

check_pkg_config x11 libx11-dev
check_pkg_config xtst libxtst-dev
check_pkg_config libcurl libcurl4-openssl-dev
check_pkg_config libpng libpng-dev

echo "All dependencies satisfied."

# Create build directory
mkdir -p "${BUILD_DIR}"
cd "${BUILD_DIR}"

# Configure
echo "Configuring build..."
cmake .. \
    -DCMAKE_BUILD_TYPE=Release \
    -DBUILD_STATIC=${BUILD_STATIC} \
    -DBUILD_DEB=${BUILD_DEB}

# Build
echo "Building..."
cmake --build . -j$(nproc)

echo ""
echo "Build complete!"
echo "Binary: ${BUILD_DIR}/ui-tars-agent"

# Build Debian package if enabled
if [ "${BUILD_DEB}" = "ON" ]; then
    echo ""
    echo "Creating Debian package..."
    cpack -G DEB
    echo "Package: ${BUILD_DIR}/ui-tars-agent-*.deb"
fi

echo ""
echo "To install:"
if [ "${BUILD_DEB}" = "ON" ]; then
    echo "  sudo dpkg -i ${BUILD_DIR}/ui-tars-agent-*.deb"
else
    echo "  sudo cp ${BUILD_DIR}/ui-tars-agent /usr/bin/"
fi
