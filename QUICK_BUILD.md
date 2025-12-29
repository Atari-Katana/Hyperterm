# Quick Build Instructions for Hyperterm

## What I've Done For You:
✅ Downloaded real stb_image.h (replaced placeholder)
✅ Created build directory structure

## What You Need To Do:

### Step 1: Install Missing Dependencies

Open a terminal and run:

```bash
sudo apt-get update
sudo apt-get install -y \
    build-essential \
    cmake \
    libglfw3-dev \
    libfreetype6-dev \
    libvulkan-dev \
    vulkan-tools \
    pkg-config
```

### Step 2: Verify Vulkan Installation

```bash
# Check if Vulkan is accessible
vulkaninfo --summary 2>/dev/null && echo "✓ Vulkan found" || echo "✗ Vulkan not found"

# If not found, try:
export VK_ICD_FILENAMES=/usr/share/vulkan/icd.d/nvidia_icd.json
# (or your GPU's ICD file)
```

### Step 3: Build the Project

```bash
cd /home/davidjackson/hyperterm
mkdir -p build
cd build
cmake ..
make -j$(nproc)
```

### Step 4: Run Hyperterm

```bash
./hyperterm
```

## If CMake Still Can't Find Vulkan:

Try setting the paths explicitly:

```bash
# Find where Vulkan headers are
find /usr/include -name "vulkan" -type d 2>/dev/null
find /usr/lib* -name "libvulkan.so*" 2>/dev/null

# Then set environment variables (adjust paths as needed):
export Vulkan_INCLUDE_DIR=/usr/include/vulkan
export Vulkan_LIBRARY=/usr/lib/x86_64-linux-gnu/libvulkan.so

# Run cmake with explicit paths:
cmake -DVulkan_INCLUDE_DIR=/usr/include/vulkan \
      -DVulkan_LIBRARY=/usr/lib/x86_64-linux-gnu/libvulkan.so ..
```

## Alternative: Use Package Manager Vulkan (Simpler)

If the above doesn't work, the system may have Vulkan but CMake needs help finding it:

```bash
# Install development packages
sudo apt-get install -y libvulkan-dev vulkan-validationlayers

# Then try building again
cd /home/davidjackson/hyperterm/build
cmake ..
make
```

## Troubleshooting

- **"Could NOT find Vulkan"**: Install `libvulkan-dev`
- **"Could NOT find glfw3"**: Install `libglfw3-dev`  
- **"Could NOT find freetype2"**: Install `libfreetype6-dev`
- **Compilation errors**: Check that you have C++17 compiler (g++ 7+ or clang++ 5+)

## Quick One-Liner Build (after dependencies installed):

```bash
cd /home/davidjackson/hyperterm && \
mkdir -p build && cd build && \
cmake .. && \
make -j$(nproc) && \
echo "✓ Build complete! Run ./hyperterm"
```
