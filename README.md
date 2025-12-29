# Hyperterm

A graphics-accelerated terminal emulator built with C++ and Vulkan, featuring image backgrounds, tabbed interface, and window tiling capabilities.

## Features

- **Graphics Acceleration**: Powered by Vulkan for high-performance rendering
- **Image Backgrounds**: Support for PNG/JPEG background images
- **Multiple Tabs**: Create and manage multiple terminal sessions
- **Menu Bar**: Easy access to settings and window tiling
- **Automatic Tiling**: One-click window tiling for organized layouts
- **Antique Serif Font**: Beautiful typography for a classic terminal experience

## Building

### Prerequisites

- CMake 3.15 or higher
- Vulkan SDK 1.3 or higher
- GLFW3
- FreeType2
- C++17 compatible compiler

### Build Instructions

```bash
mkdir build
cd build
cmake ..
make
```

## Usage

Run `./hyperterm` to start the terminal emulator.

### Keyboard Shortcuts

- `Ctrl+T`: New tab
- `Ctrl+W`: Close current tab
- `Ctrl+Tab`: Switch to next tab
- `Ctrl+Shift+Tab`: Switch to previous tab

### Menu Options

- **File**: New tab, close tab, quit
- **Settings**: Configure terminal appearance and behavior
- **Tile**: Automatically tile all open terminal windows

## License

MIT

