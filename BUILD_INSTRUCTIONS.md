# Hyperterm Build Instructions

## Step-by-Step Build Guide

### Prerequisites Check

First, check what's already installed:

```bash
# Check for CMake
cmake --version

# Check for C++ compiler
g++ --version

# Check for GLFW3
pkg-config --modversion glfw3 || echo "GLFW3 not installed"

# Check for FreeType2
pkg-config --modversion freetype2 || echo "FreeType2 not installed"

# Check for Vulkan
vulkaninfo --summary 2>/dev/null || echo "Vulkan SDK not installed"
```

### Step 1: Install System Dependencies (Ubuntu/Debian)

```bash
sudo apt-get update
sudo apt-get install -y \
    build-essential \
    cmake \
    libglfw3-dev \
    libfreetype6-dev \
    pkg-config
```

### Step 2: Install Vulkan SDK

The Vulkan SDK needs to be installed separately:

#### Option A: Install from Package Manager (Simpler, but may be older version)

```bash
sudo apt-get install -y \
    vulkan-tools \
    libvulkan-dev \
    vulkan-validationlayers-dev
```

#### Option B: Install Latest Vulkan SDK (Recommended)

1. Download the Vulkan SDK from: https://vulkan.lunarg.com/sdk/home#linux
2. Extract and run the installer:
   ```bash
   wget https://sdk.lunarg.com/sdk/download/1.3.xxx.x/linux/vulkansdk-linux-x86_64-1.3.xxx.x.tar.gz
   tar -xf vulkansdk-linux-x86_64-1.3.xxx.x.tar.gz
   source 1.3.xxx.x/setup-env.sh
   ```

### Step 3: Download Real stb_image.h (Required for Image Loading)

```bash
cd /home/davidjackson/hyperterm/src/renderer
wget https://raw.githubusercontent.com/nothings/stb/master/stb_image.h
```

### Step 4: Build the Project

```bash
cd /home/davidjackson/hyperterm
mkdir -p build
cd build
cmake ..
make
```

### Step 5: Run Hyperterm

```bash
./hyperterm
```

## Troubleshooting

### If CMake can't find Vulkan:

Set the Vulkan SDK path explicitly:
```bash
export VULKAN_SDK=/path/to/vulkan/sdk
export PATH=$VULKAN_SDK/bin:$PATH
export LD_LIBRARY_PATH=$VULKAN_SDK/lib:$LD_LIBRARY_PATH
```

Then run cmake again:
```bash
cmake -DVulkan_INCLUDE_DIR=$VULKAN_SDK/include \
      -DVulkan_LIBRARY=$VULKAN_SDK/lib/libvulkan.so ..
```

### If compilation fails:

1. Check that all dependencies are installed
2. Verify C++17 support: `g++ -std=c++17 --version`
3. Check CMake output for missing packages

### Common Issues:

- **Vulkan not found**: Install Vulkan SDK (see Step 2)
- **GLFW3 not found**: `sudo apt-get install libglfw3-dev`
- **FreeType not found**: `sudo apt-get install libfreetype6-dev`
- **stb_image errors**: Download the real stb_image.h (see Step 3)

## Quick Build Script

Save this as `build.sh` and run `chmod +x build.sh && ./build.sh`:

```bash
#!/bin/bash
set -e

echo "Checking dependencies..."
cmake --version || { echo "CMake not found!"; exit 1; }

echo "Downloading stb_image.h..."
cd src/renderer
if [ ! -f stb_image.h ] || grep -q "Placeholder" stb_image.h; then
    wget -q https://raw.githubusercontent.com/nothings/stb/master/stb_image.h -O stb_image.h
    echo "Downloaded stb_image.h"
fi
cd ../..

echo "Building..."
mkdir -p build
cd build
cmake ..
make -j$(nproc)

echo "Build complete! Run ./hyperterm to start"
