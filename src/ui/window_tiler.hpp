#pragma once

#include <vector>
#include <cstdint>

struct TileRect {
    float x;
    float y;
    float width;
    float height;
    
    TileRect(float x = 0, float y = 0, float w = 0, float h = 0)
        : x(x), y(y), width(w), height(h) {}
};

class WindowTiler {
public:
    WindowTiler();
    
    void tileWindows(size_t count, float totalWidth, float totalHeight, float menuBarHeight, std::vector<TileRect>& tiles);
    
    enum TileMode {
        GRID_2x2,
        GRID_3x2,
        HORIZONTAL_SPLIT,
        VERTICAL_SPLIT,
        AUTO
    };
    
    void setTileMode(TileMode mode) { tileMode_ = mode; }
    TileMode getTileMode() const { return tileMode_; }
    
private:
    TileMode tileMode_;
    
    void tileGrid(size_t count, float width, float height, float menuBarHeight, std::vector<TileRect>& tiles);
    void tileHorizontal(size_t count, float width, float height, float menuBarHeight, std::vector<TileRect>& tiles);
    void tileVertical(size_t count, float width, float height, float menuBarHeight, std::vector<TileRect>& tiles);
};

