#!/bin/bash
# Build the Vulkan diagnostic test

echo "Building Vulkan diagnostic test..."
g++ -std=c++17 vulkan_test.cpp -o vulkan_test -lglfw -lvulkan -lpthread

if [ $? -eq 0 ]; then
    echo "✓ Build successful!"
    echo ""
    echo "Run the test with: ./vulkan_test"
else
    echo "✗ Build failed"
    exit 1
fi
