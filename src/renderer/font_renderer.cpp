#include "font_renderer.hpp"
#include "vulkan_renderer.hpp"
#include <stdexcept>
#include <algorithm>
#include <cstring>

FontRenderer::FontRenderer(VkDevice device, VkPhysicalDevice physicalDevice, VkQueue graphicsQueue, VkCommandPool commandPool)
    : device_(device), physicalDevice_(physicalDevice), graphicsQueue_(graphicsQueue), commandPool_(commandPool), 
      fontSize_(16), lineHeight_(20), ftLibrary_(nullptr), ftFace_(nullptr), renderer_(nullptr),
      atlasImage_(VK_NULL_HANDLE), atlasMemory_(VK_NULL_HANDLE), atlasView_(VK_NULL_HANDLE),
      atlasWidth_(1024), atlasHeight_(1024), atlasX_(0), atlasY_(0), atlasRowHeight_(0) {
    if (FT_Init_FreeType(&ftLibrary_)) {
        throw std::runtime_error("Failed to initialize FreeType");
    }
    initAtlas();
}

FontRenderer::~FontRenderer() {
    cleanup();
    if (ftFace_) {
        FT_Done_Face(ftFace_);
    }
    FT_Done_FreeType(ftLibrary_);
}

bool FontRenderer::loadFont(const std::string& fontPath, uint32_t fontSize) {
    fontPath_ = fontPath;
    fontSize_ = fontSize;
    
    if (ftFace_) {
        FT_Done_Face(ftFace_);
        ftFace_ = nullptr;
    }
    
    // Clear existing glyphs and reset atlas
    cleanup(); // Clean up old atlas, if any
    initAtlas(); // Create new atlas
    atlasX_ = 0;
    atlasY_ = 0;
    atlasRowHeight_ = 0;
    glyphs_.clear(); // Clear old glyphs
    
    if (FT_New_Face(ftLibrary_, fontPath.c_str(), 0, &ftFace_)) {
        return false;
    }
    
    FT_Set_Pixel_Sizes(ftFace_, 0, fontSize);
    lineHeight_ = (ftFace_->size->metrics.height >> 6);
    
    for (char32_t c = 32; c < 127; c++) {
        getGlyph(c);
    }
    
    return true;
}

AtlasGlyph* FontRenderer::getGlyph(char32_t codepoint) {
    if (glyphs_.find(codepoint) == glyphs_.end()) {
        createGlyph(codepoint);
    }
    return &glyphs_[codepoint];
}

void FontRenderer::createGlyph(char32_t codepoint) {
    if (!ftFace_) {
        return;
    }
    
    if (FT_Load_Char(ftFace_, codepoint, FT_LOAD_RENDER)) {
        AtlasGlyph glyph{};
        glyph.u0 = 0.0f; glyph.v0 = 0.0f; glyph.u1 = 0.0f; glyph.v1 = 0.0f;
        glyph.width = fontSize_ / 2;
        glyph.height = fontSize_;
        glyph.bearingX = 0;
        glyph.bearingY = fontSize_;
        glyph.advance = fontSize_;
        glyphs_[codepoint] = glyph;
        return;
    }
    
    FT_GlyphSlot slot = ftFace_->glyph;
    FT_Bitmap& bitmap = slot->bitmap;

    if (bitmap.width == 0 || bitmap.rows == 0) {
        AtlasGlyph glyph{};
        glyph.width = 0;
        glyph.height = 0;
        glyph.bearingX = slot->bitmap_left;
        glyph.bearingY = slot->bitmap_top;
        glyph.advance = slot->advance.x >> 6;
        glyphs_[codepoint] = glyph;
        return;
    }

    if (atlasX_ + bitmap.width > atlasWidth_) {
        atlasX_ = 0;
        atlasY_ += atlasRowHeight_;
        atlasRowHeight_ = 0;
    }
    if (atlasY_ + bitmap.rows > atlasHeight_) {
        AtlasGlyph glyph{};
        glyphs_[codepoint] = glyph; 
        return;
    }

    std::vector<uint8_t> rgbaBitmap(bitmap.width * bitmap.rows * 4);
    for (uint32_t y = 0; y < bitmap.rows; ++y) {
        for (uint32_t x = 0; x < bitmap.width; ++x) {
            uint8_t alpha = bitmap.buffer[y * bitmap.pitch + x];
            rgbaBitmap[(y * bitmap.width + x) * 4 + 0] = 255;
            rgbaBitmap[(y * bitmap.width + x) * 4 + 1] = 255;
            rgbaBitmap[(y * bitmap.width + x) * 4 + 2] = 255;
            rgbaBitmap[(y * bitmap.width + x) * 4 + 3] = alpha;
        }
    }
    
    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    renderer_->createBuffer(bitmap.width * bitmap.rows * 4, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

    void* data;
    vkMapMemory(device_, stagingBufferMemory, 0, bitmap.width * bitmap.rows * 4, 0, &data);
    memcpy(data, rgbaBitmap.data(), bitmap.width * bitmap.rows * 4);
    vkUnmapMemory(device_, stagingBufferMemory);

    VkCommandBuffer cmd = renderer_->beginSingleTimeCommands();
    VkBufferImageCopy region{};
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.layerCount = 1;
    region.imageOffset = { (int32_t)atlasX_, (int32_t)atlasY_, 0 };
    region.imageExtent = { bitmap.width, bitmap.rows, 1 };
    vkCmdCopyBufferToImage(cmd, stagingBuffer, atlasImage_, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
    renderer_->endSingleTimeCommands(cmd);

    vkDestroyBuffer(device_, stagingBuffer, nullptr);
    vkFreeMemory(device_, stagingBufferMemory, nullptr);
    
    AtlasGlyph glyph{};
    glyph.u0 = (float)atlasX_ / atlasWidth_;
    glyph.v0 = (float)atlasY_ / atlasHeight_;
    glyph.u1 = (float)(atlasX_ + bitmap.width) / atlasWidth_;
    glyph.v1 = (float)(atlasY_ + bitmap.rows) / atlasHeight_;
    glyph.width = bitmap.width;
    glyph.height = bitmap.rows;
    glyph.bearingX = slot->bitmap_left;
    glyph.bearingY = slot->bitmap_top;
    glyph.advance = slot->advance.x >> 6;
    glyphs_[codepoint] = glyph;

    atlasX_ += bitmap.width;
    atlasRowHeight_ = std::max(atlasRowHeight_, bitmap.rows);
}

void FontRenderer::renderCharacter(float x, float y, char32_t c, float r, float g, float b) {
    if (!renderer_) return;
    
    AtlasGlyph* glyph = getGlyph(c);
    if (glyph && glyph->width > 0 && glyph->height > 0) {
        float glyphX = x + glyph->bearingX;
        float glyphY = y - (glyph->height - glyph->bearingY);
        
        renderer_->renderQuad(
            glyphX, glyphY,
            static_cast<float>(glyph->width),
            static_cast<float>(glyph->height),
            atlasView_, // Use the atlas texture view
            r, g, b, 1.0f,
            glyph->u0, glyph->v0, glyph->u1, glyph->v1 // Pass texture coordinates
        );
    }
}

void FontRenderer::renderString(float x, float y, const std::string& text, float r, float g, float b) {
    if (!renderer_) return;
    
    float currentX = x;
    
    for (char c : text) {
        // This is not UTF-8 safe, but it's what the old code did.
        // The UI code that calls this only uses ASCII.
        // For proper UTF-8 handling here, we'd need a UTF-8 decoder.
        // For now, we assume ASCII for renderString for UI elements.
        AtlasGlyph* glyph = getGlyph(static_cast<char32_t>(c));
        if (glyph) {
            renderCharacter(currentX, y, static_cast<char32_t>(c), r, g, b);
            currentX += glyph->advance;
        }
    }
}

uint32_t FontRenderer::getTextWidth(const std::string& text) const {
    uint32_t width = 0;
    for (char c : text) {
        auto it = glyphs_.find(static_cast<char32_t>(c));
        if (it != glyphs_.end()) {
            width += it->second.advance;
        }
    }
    return width;
}

void FontRenderer::initAtlas() {
    if (!renderer_) return;
    std::vector<unsigned char> emptyData(atlasWidth_ * atlasHeight_ * 4, 0); // Transparent black
    renderer_->createTexture(atlasWidth_, atlasHeight_, emptyData.data(), atlasImage_, atlasMemory_, atlasView_);
}

void FontRenderer::cleanup() {
    if (renderer_ && atlasView_ != VK_NULL_HANDLE) {
        renderer_->destroyTexture(atlasImage_, atlasMemory_, atlasView_);
        atlasImage_ = VK_NULL_HANDLE;
        atlasMemory_ = VK_NULL_HANDLE;
        atlasView_ = VK_NULL_HANDLE;
    }
    glyphs_.clear();
}


