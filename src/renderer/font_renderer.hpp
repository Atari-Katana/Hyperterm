#pragma once

#include <vulkan/vulkan.h>
#include <ft2build.h>
#include FT_FREETYPE_H
#include <string>
#include <unordered_map>
#include <vector>
#include <cstdint>

class VulkanRenderer;

struct AtlasGlyph {
    float u0, v0, u1, v1; // Texture coordinates in the atlas
    uint32_t width;
    uint32_t height;
    int32_t bearingX;
    int32_t bearingY;
    uint32_t advance;
};

class FontRenderer {
public:
    FontRenderer(VkDevice device, VkPhysicalDevice physicalDevice, VkQueue graphicsQueue, VkCommandPool commandPool);
    ~FontRenderer();
    
    bool loadFont(const std::string& fontPath, uint32_t fontSize);
    void cleanup();
    
    AtlasGlyph* getGlyph(char32_t codepoint);
    void renderString(float x, float y, const std::string& text, float r = 1.0f, float g = 1.0f, float b = 1.0f);
    void renderCharacter(float x, float y, char32_t c, float r = 1.0f, float g = 1.0f, float b = 1.0f);
    
    uint32_t getTextWidth(const std::string& text) const;
    uint32_t getLineHeight() const { return lineHeight_; }
    
    void setRenderer(VulkanRenderer* renderer) { renderer_ = renderer; }
    
private:
    VkDevice device_;
    VkPhysicalDevice physicalDevice_;
    VkQueue graphicsQueue_;
    VkCommandPool commandPool_;
    
    std::unordered_map<char32_t, AtlasGlyph> glyphs_;
    uint32_t fontSize_;
    uint32_t lineHeight_;
    std::string fontPath_;
    
    FT_Library ftLibrary_;
    FT_Face ftFace_;
    VulkanRenderer* renderer_;
    
    // Glyph atlas members
    VkImage atlasImage_;
    VkDeviceMemory atlasMemory_;
    VkImageView atlasView_;
    uint32_t atlasWidth_;
    uint32_t atlasHeight_;
    uint32_t atlasX_;
    uint32_t atlasY_;
    uint32_t atlasRowHeight_;
    
    void createGlyph(char32_t codepoint);
    void createImage(uint32_t width, uint32_t height, const void* data, VkImage& image, VkDeviceMemory& memory, VkImageView& view);
    void initAtlas();
};

