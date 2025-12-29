#pragma once

#include <string>
#include <vector>
#include <cstdint>

struct ImageData {
    std::vector<uint8_t> pixels;
    uint32_t width;
    uint32_t height;
    uint32_t channels;
    
    ImageData() : width(0), height(0), channels(0) {}
};

class ImageLoader {
public:
    static ImageData loadImage(const std::string& path);
    static void freeImage(ImageData& image);
};

