#pragma once

#include <functional>
#include <string>
#include <vector>

// Forward declarations
class VulkanRenderer;
class FontRenderer;

class MenuBar {
public:
    MenuBar();
    
    void render(float width, float height);
    void setRenderer(VulkanRenderer* renderer);
    void setFontRenderer(FontRenderer* fontRenderer);
    
    // Minimal callbacks for compilation
    std::function<void()> onNewTab;
    std::function<void()> onCloseTab;
    std::function<void()> onQuit;
    std::function<void()> onSettings;
    std::function<void()> onTile;
    
    bool handleClick(float x, float y);
    bool handleKey(int key, int mods);
    
private:
    struct MenuItem {
        std::string label;
        std::function<void()> callback;
        float x, y, width, height;
    };
    
    std::vector<MenuItem> menuItems_;
    float menuBarHeight_;
    
    VulkanRenderer* renderer_ = nullptr;
    FontRenderer* fontRenderer_ = nullptr;
    
    void createMenuItems();
};