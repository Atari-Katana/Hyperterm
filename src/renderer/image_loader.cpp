#include "image_loader.hpp"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include <stdexcept>
#include <cstring>

ImageData ImageLoader::loadImage(const std::string& path) {
    ImageData result;
    int width, height, channels;
    
    unsigned char* data = stbi_load(path.c_str(), &width, &height, &channels, STBI_rgb_alpha);
    if (!data) {
        // If stb_image stub returns nullptr, provide a helpful error message
        throw std::runtime_error("Failed to load image: " + path + 
            " (Note: Replace src/renderer/stb_image.h with the real version from https://github.com/nothings/stb)");
    }
    
    result.width = static_cast<uint32_t>(width);
    result.height = static_cast<uint32_t>(height);
    result.channels = 4; // STBI_rgb_alpha always returns 4 channels
    
    size_t imageSize = result.width * result.height * result.channels;
    result.pixels.resize(imageSize);
    std::memcpy(result.pixels.data(), data, imageSize);
    
    stbi_image_free(data);
    
    return result;
}

void ImageLoader::freeImage(ImageData& image) {
    image.pixels.clear();
    image.width = 0;
    image.height = 0;
    image.channels = 0;
}

