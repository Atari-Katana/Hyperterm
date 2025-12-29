#include "menu_bar.hpp"
#include "../renderer/vulkan_renderer.hpp"
#include "../renderer/font_renderer.hpp"
#include <algorithm>

MenuBar::MenuBar() : menuBarHeight_(30.0f) {
    createMenuItems();
}

void MenuBar::setRenderer(VulkanRenderer* renderer) {
    renderer_ = renderer;
}

void MenuBar::setFontRenderer(FontRenderer* fontRenderer) {
    fontRenderer_ = fontRenderer;
}

void MenuBar::createMenuItems() {
    menuItems_.clear();
    
    MenuItem fileMenu;
    fileMenu.label = "File";
    fileMenu.x = 10.0f;
    fileMenu.y = 0.0f;
    fileMenu.width = 50.0f;
    fileMenu.height = menuBarHeight_;
    menuItems_.push_back(fileMenu);
    
    MenuItem settingsMenu;
    settingsMenu.label = "Settings";
    settingsMenu.x = 70.0f;
    settingsMenu.y = 0.0f;
    settingsMenu.width = 80.0f;
    settingsMenu.height = menuBarHeight_;
    settingsMenu.callback = onSettings;
    menuItems_.push_back(settingsMenu);
    
    MenuItem tileButton;
    tileButton.label = "Tile";
    tileButton.x = 160.0f;
    tileButton.y = 0.0f;
    tileButton.width = 50.0f;
    tileButton.height = menuBarHeight_;
    tileButton.callback = onTile;
    menuItems_.push_back(tileButton);
}

void MenuBar::render(float width, [[maybe_unused]] float height) {
    if (!renderer_) return;
    
    // Render menu bar background (dark gray)
    renderer_->renderQuad(0.0f, 0.0f, width, menuBarHeight_, VK_NULL_HANDLE, 0.25f, 0.25f, 0.25f, 1.0f);
    
    // Render menu items
    if (fontRenderer_) {
        for (const auto& item : menuItems_) {
            // Render item background on hover would go here
            // For now, just render text
            fontRenderer_->renderString(item.x + 5.0f, item.y + 5.0f, item.label, 0.9f, 0.9f, 0.9f);
        }
    }
}

bool MenuBar::handleClick(float x, float y) {
    if (y > menuBarHeight_) return false;
    
    for (auto& item : menuItems_) {
        if (x >= item.x && x <= item.x + item.width &&
            y >= item.y && y <= item.y + item.height) {
            if (item.callback) {
                item.callback();
            }
            return true;
        }
    }
    return false;
}

bool MenuBar::handleKey(int key, int mods) {
    // Handle keyboard shortcuts
    // Ctrl+T for new tab, Ctrl+W for close tab, etc.
    if (mods & 0x0004) { // Ctrl modifier
        if (key == 84) { // T
            if (onNewTab) onNewTab();
            return true;
        } else if (key == 87) { // W
            if (onCloseTab) onCloseTab();
            return true;
        }
    }
    return false;
}

