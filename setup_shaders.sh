#!/bin/bash
# Hyperterm Shader Setup Script
# This script installs shader compiler and rebuilds the project

echo "=========================================="
echo "Hyperterm Shader Setup"
echo "=========================================="
echo

# Check if running with sudo
if [ "$EUID" -ne 0 ]; then
    echo "Installing shader compiler tools..."
    sudo apt update
    sudo apt install -y glslang-tools
else
    echo "Installing shader compiler tools..."
    apt update
    apt install -y glslang-tools
fi

echo
echo "Shader compiler installed successfully!"
echo

# Verify installation
if command -v glslangValidator &> /dev/null; then
    echo "✓ glslangValidator found: $(which glslangValidator)"
elif command -v glslc &> /dev/null; then
    echo "✓ glslc found: $(which glslc)"
else
    echo "✗ Warning: No shader compiler found"
    exit 1
fi

echo
echo "Reconfiguring CMake build..."
cd "$(dirname "$0")"
rm -rf build
mkdir build
cd build
cmake ..

echo
echo "Building Hyperterm with shader compilation..."
cmake --build .

echo
echo "=========================================="
echo "Setup Complete!"
echo "=========================================="
echo
echo "You can now run Hyperterm with: ./build/Hyperterm"
echo
