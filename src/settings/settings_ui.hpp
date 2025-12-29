#pragma once

#include "settings.hpp"
#include "../renderer/vulkan_renderer.hpp"
#include "../renderer/font_renderer.hpp"
#include <functional>
#include <vector>
#include <string>

class SettingsUI {
public:
    SettingsUI(Settings* settings);
    
    void render(float width, float height);
    bool handleClick(float x, float y);
    bool handleKey(int key, int action);  // action: 0=release, 1=press, 2=repeat
    bool isVisible() const { return visible_; }
    void show();
    void hide() { visible_ = false; }
    
    void setRenderer(VulkanRenderer* renderer) { renderer_ = renderer; }
    void setFontRenderer(FontRenderer* fontRenderer) { fontRenderer_ = fontRenderer; }
    
    std::function<void()> onClose;
    
private:
    Settings* settings_;
    VulkanRenderer* renderer_;
    FontRenderer* fontRenderer_;
    bool visible_;
    float dialogWidth_;
    float dialogHeight_;
    float dialogX_;
    float dialogY_;
    
    struct FontEntry {
        std::string name;      // Display name
        std::string path;      // Full path
        bool isSelected;
    };
    
    std::vector<FontEntry> availableFonts_;
    int selectedFontIndex_;
    int scrollOffset_;
    float fontListStartY_;
    float fontListEndY_;
    float itemHeight_;
    
    bool fontSizeChanged_;
    int fontSize_;
    
    void discoverFonts();
    void renderDialog(float width, float height);
    void renderFontList(float x, float y, float width, float height);
    void renderFontSizeControl(float x, float y, float width);
    void renderButtons(float x, float y, float width);
    bool isPointInRect(float x, float y, float rectX, float rectY, float rectW, float rectH);
    void selectFont(int index);
    void applySettings();
};

