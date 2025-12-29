#include "window_tiler.hpp"
#include <cmath>
#include <algorithm>

WindowTiler::WindowTiler() : tileMode_(AUTO) {
}

void WindowTiler::tileWindows(size_t count, float totalWidth, float totalHeight, float menuBarHeight, std::vector<TileRect>& tiles) {
    tiles.clear();
    if (count == 0) return;
    
    float availableHeight = totalHeight - menuBarHeight;
    float startY = menuBarHeight;
    
    switch (tileMode_) {
        case HORIZONTAL_SPLIT:
            tileHorizontal(count, totalWidth, availableHeight, startY, tiles);
            break;
        case VERTICAL_SPLIT:
            tileVertical(count, totalWidth, availableHeight, startY, tiles);
            break;
        case AUTO:
        case GRID_2x2:
        case GRID_3x2:
        default:
            tileGrid(count, totalWidth, availableHeight, startY, tiles);
            break;
    }
}

void WindowTiler::tileGrid(size_t count, float width, float height, float startY, std::vector<TileRect>& tiles) {
    if (count == 0) return;
    
    size_t cols = static_cast<size_t>(std::ceil(std::sqrt(count)));
    size_t rows = static_cast<size_t>(std::ceil(static_cast<float>(count) / cols));
    
    float cellWidth = width / cols;
    float cellHeight = height / rows;
    
    for (size_t i = 0; i < count; i++) {
        size_t col = i % cols;
        size_t row = i / cols;
        
        TileRect rect;
        rect.x = col * cellWidth;
        rect.y = startY + row * cellHeight;
        rect.width = cellWidth;
        rect.height = cellHeight;
        
        tiles.push_back(rect);
    }
}

void WindowTiler::tileHorizontal(size_t count, float width, float height, float startY, std::vector<TileRect>& tiles) {
    if (count == 0) return;
    
    float cellHeight = height / count;
    
    for (size_t i = 0; i < count; i++) {
        TileRect rect;
        rect.x = 0.0f;
        rect.y = startY + i * cellHeight;
        rect.width = width;
        rect.height = cellHeight;
        
        tiles.push_back(rect);
    }
}

void WindowTiler::tileVertical(size_t count, float width, float height, float startY, std::vector<TileRect>& tiles) {
    if (count == 0) return;
    
    float cellWidth = width / count;
    
    for (size_t i = 0; i < count; i++) {
        TileRect rect;
        rect.x = i * cellWidth;
        rect.y = startY;
        rect.width = cellWidth;
        rect.height = height;
        
        tiles.push_back(rect);
    }
}

