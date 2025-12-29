#include "settings_ui.hpp"
#include "../renderer/vulkan_renderer.hpp"
#include "../renderer/font_renderer.hpp"
#include <algorithm>
#include <iostream>
#include <cstdlib>
#include <sys/stat.h>
#include <dirent.h>
#include <cstring>

// Simple filesystem wrapper for font discovery
namespace {
    bool isDirectory(const std::string& path) {
        struct stat buffer;
        if (stat(path.c_str(), &buffer) == 0) {
            return S_ISDIR(buffer.st_mode);
        }
        return false;
    }
    
    std::vector<std::string> listDirectory(const std::string& path) {
        std::vector<std::string> files;
        DIR* dir = opendir(path.c_str());
        if (dir == nullptr) return files;
        
        struct dirent* entry;
        while ((entry = readdir(dir)) != nullptr) {
            if (entry->d_name[0] != '.') {  // Skip hidden files
                files.push_back(entry->d_name);
            }
        }
        closedir(dir);
        return files;
    }
    
    std::string getExtension(const std::string& filename) {
        size_t pos = filename.find_last_of('.');
        if (pos != std::string::npos) {
            std::string ext = filename.substr(pos);
            std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
            return ext;
        }
        return "";
    }
}

SettingsUI::SettingsUI(Settings* settings) 
    : settings_(settings), renderer_(nullptr), fontRenderer_(nullptr),
      visible_(false), dialogWidth_(600.0f), dialogHeight_(500.0f),
      dialogX_(0.0f), dialogY_(0.0f),
      selectedFontIndex_(-1), scrollOffset_(0),
      fontListStartY_(0.0f), fontListEndY_(0.0f), itemHeight_(30.0f),
      fontSizeChanged_(false), fontSize_(16) {
}

void SettingsUI::show() {
    visible_ = true;
    discoverFonts();
    
    // Get current settings
    fontSize_ = settings_->getFontSize();
    std::string currentFontPath = settings_->getFontPath();
    
    // Find current font in list
    selectedFontIndex_ = -1;
    for (size_t i = 0; i < availableFonts_.size(); i++) {
        if (availableFonts_[i].path == currentFontPath || 
            availableFonts_[i].name.find(currentFontPath) != std::string::npos) {
            selectedFontIndex_ = static_cast<int>(i);
            availableFonts_[i].isSelected = true;
            break;
        }
    }
}

void SettingsUI::discoverFonts() {
    availableFonts_.clear();
    
    // Check fonts directory (relative to current working directory)
    std::string fontsDir = "fonts";
    if (!isDirectory(fontsDir)) {
        // Try absolute path from project root
        const char* home = getenv("HOME");
        if (home) {
            fontsDir = std::string(home) + "/hyperterm/fonts";
            if (!isDirectory(fontsDir)) {
                fontsDir = "./fonts";  // Fallback
            }
        }
    }
    
    if (isDirectory(fontsDir)) {
        auto files = listDirectory(fontsDir);
        for (const auto& filename : files) {
            std::string ext = getExtension(filename);
            if (ext == ".ttf" || ext == ".otf") {
                FontEntry font;
                font.name = filename;
                if (fontsDir.back() == '/') {
                    font.path = fontsDir + filename;
                } else {
                    font.path = fontsDir + "/" + filename;
                }
                font.isSelected = false;
                availableFonts_.push_back(font);
            }
        }
    }
    
    // Sort fonts by name
    std::sort(availableFonts_.begin(), availableFonts_.end(),
              [](const FontEntry& a, const FontEntry& b) {
                  return a.name < b.name;
              });
}

void SettingsUI::render(float width, float height) {
    if (!visible_ || !renderer_) return;
    
    // Center dialog
    dialogX_ = (width - dialogWidth_) / 2.0f;
    dialogY_ = (height - dialogHeight_) / 2.0f;
    
    renderDialog(width, height);
}

void SettingsUI::renderDialog([[maybe_unused]] float width, [[maybe_unused]] float height) {
    if (!renderer_) return;
    
    float headerHeight = 40.0f;
    float footerHeight = 60.0f;
    float contentHeight = dialogHeight_ - headerHeight - footerHeight;
    
    // Render dialog background (dark gray)
    renderer_->renderQuad(dialogX_, dialogY_, dialogWidth_, dialogHeight_, VK_NULL_HANDLE, 0.2f, 0.2f, 0.2f, 0.95f);
    
    // Render header (darker background)
    renderer_->renderQuad(dialogX_, dialogY_, dialogWidth_, headerHeight, VK_NULL_HANDLE, 0.15f, 0.15f, 0.15f, 1.0f);
    
    // Render "Settings" title
    if (fontRenderer_) {
        fontRenderer_->renderString(dialogX_ + 20.0f, dialogY_ + 10.0f, "Settings", 1.0f, 1.0f, 1.0f);
    }
    
    // Render font list area background
    fontListStartY_ = dialogY_ + headerHeight + 10.0f;
    fontListEndY_ = dialogY_ + headerHeight + contentHeight - 10.0f;
    float listAreaHeight = fontListEndY_ - fontListStartY_;
    renderer_->renderQuad(dialogX_ + 20.0f, fontListStartY_, dialogWidth_ - 40.0f, listAreaHeight, VK_NULL_HANDLE, 0.1f, 0.1f, 0.1f, 1.0f);
    
    renderFontList(dialogX_ + 20.0f, fontListStartY_, dialogWidth_ - 40.0f, listAreaHeight);
    
    // Render font size control
    renderFontSizeControl(dialogX_ + 20.0f, fontListEndY_ + 10.0f, dialogWidth_ - 40.0f);
    
    // Render buttons
    renderButtons(dialogX_, dialogY_ + dialogHeight_ - footerHeight, dialogWidth_);
}

void SettingsUI::renderFontList(float x, float y, float width, float height) {
    if (!fontRenderer_) return;
    
    float currentY = y - (scrollOffset_ * itemHeight_);
    
    for (size_t i = 0; i < availableFonts_.size(); i++) {
        if (currentY + itemHeight_ < y) {
            currentY += itemHeight_;
            continue; // Above visible area
        }
        if (currentY > y + height) {
            break; // Below visible area
        }
        
        const auto& font = availableFonts_[i];
        
        // Highlight selected font
        if (font.isSelected || static_cast<int>(i) == selectedFontIndex_) {
            // Render selection background (blue highlight)
            renderer_->renderQuad(x, currentY, width, itemHeight_, VK_NULL_HANDLE, 0.2f, 0.4f, 0.8f, 1.0f);
        }
        
        // Render font name
        if (fontRenderer_ && renderer_) {
            float textColor = (font.isSelected || static_cast<int>(i) == selectedFontIndex_) ? 1.0f : 0.9f;
            fontRenderer_->renderString(x + 10.0f, currentY + 5.0f, font.name, textColor, textColor, textColor);
        }
        
        currentY += itemHeight_;
    }
}

void SettingsUI::renderFontSizeControl(float x, float y, [[maybe_unused]] float width) {
    if (!renderer_ || !fontRenderer_) return;
    
    // Render "Font Size:" label
    fontRenderer_->renderString(x, y, "Font Size:", 0.9f, 0.9f, 0.9f);
    
    // Render decrease button
    float buttonWidth = 40.0f;
    float buttonHeight = 30.0f;
    float buttonY = y + 25.0f;
    renderer_->renderQuad(x, buttonY, buttonWidth, buttonHeight, VK_NULL_HANDLE, 0.3f, 0.3f, 0.3f, 1.0f);
    fontRenderer_->renderString(x + 15.0f, buttonY + 5.0f, "-", 1.0f, 1.0f, 1.0f);
    
    // Render size value
    float sizeX = x + buttonWidth + 20.0f;
    std::string sizeStr = std::to_string(fontSize_);
    fontRenderer_->renderString(sizeX, buttonY + 5.0f, sizeStr, 1.0f, 1.0f, 1.0f);
    
    // Render increase button
    float increaseX = sizeX + 60.0f;
    renderer_->renderQuad(increaseX, buttonY, buttonWidth, buttonHeight, VK_NULL_HANDLE, 0.3f, 0.3f, 0.3f, 1.0f);
    fontRenderer_->renderString(increaseX + 15.0f, buttonY + 5.0f, "+", 1.0f, 1.0f, 1.0f);
}

void SettingsUI::renderButtons(float x, float y, float width) {
    if (!renderer_ || !fontRenderer_) return;
    
    float buttonWidth = 100.0f;
    float buttonHeight = 40.0f;
    float buttonY = y + 10.0f;
    float buttonSpacing = 20.0f;
    
    // Cancel button (left side)
    float cancelX = x + width - (buttonWidth * 2 + buttonSpacing);
    renderer_->renderQuad(cancelX, buttonY, buttonWidth, buttonHeight, VK_NULL_HANDLE, 0.4f, 0.4f, 0.4f, 1.0f);
    fontRenderer_->renderString(cancelX + 25.0f, buttonY + 10.0f, "Cancel", 1.0f, 1.0f, 1.0f);
    
    // OK/Apply button (right side)
    float okX = x + width - buttonWidth - 20.0f;
    renderer_->renderQuad(okX, buttonY, buttonWidth, buttonHeight, VK_NULL_HANDLE, 0.2f, 0.6f, 0.2f, 1.0f);
    fontRenderer_->renderString(okX + 35.0f, buttonY + 10.0f, "Apply", 1.0f, 1.0f, 1.0f);
}

bool SettingsUI::handleClick(float x, float y) {
    if (!visible_) return false;
    
    // Check if click is outside dialog
    if (!isPointInRect(x, y, dialogX_, dialogY_, dialogWidth_, dialogHeight_)) {
        hide();
        return true;
    }
    
    // Check font list clicks
    if (isPointInRect(x, y, dialogX_ + 20.0f, fontListStartY_, dialogWidth_ - 40.0f, fontListEndY_ - fontListStartY_)) {
        float relativeY = y - fontListStartY_ + (scrollOffset_ * itemHeight_);
        int clickedIndex = static_cast<int>(relativeY / itemHeight_);
        if (clickedIndex >= 0 && clickedIndex < static_cast<int>(availableFonts_.size())) {
            selectFont(clickedIndex);
            return true;
        }
    }
    
    // Check font size buttons
    float fontSizeY = fontListEndY_ + 10.0f + 25.0f;
    float buttonWidth = 40.0f;
    float decreaseX = dialogX_ + 20.0f;
    float increaseX = decreaseX + 40.0f + 20.0f + 60.0f;
    
    if (isPointInRect(x, y, decreaseX, fontSizeY, buttonWidth, 30.0f)) {
        if (fontSize_ > 8) {
            fontSize_--;
            fontSizeChanged_ = true;
        }
        return true;
    }
    
    if (isPointInRect(x, y, increaseX, fontSizeY, buttonWidth, 30.0f)) {
        if (fontSize_ < 72) {
            fontSize_++;
            fontSizeChanged_ = true;
        }
        return true;
    }
    
    // Check dialog buttons
    float buttonY = dialogY_ + dialogHeight_ - 60.0f + 10.0f;
    float buttonHeight = 40.0f;
    float cancelX = dialogX_ + dialogWidth_ - (100.0f * 2 + 20.0f);
    float okX = dialogX_ + dialogWidth_ - 100.0f - 20.0f;
    
    if (isPointInRect(x, y, cancelX, buttonY, 100.0f, buttonHeight)) {
        hide();
        return true;
    }
    
    if (isPointInRect(x, y, okX, buttonY, 100.0f, buttonHeight)) {
        applySettings();
        hide();
        if (onClose) onClose();
        return true;
    }
    
    return false;
}

bool SettingsUI::handleKey(int key, int action) {
    if (!visible_ || action != 1) return false; // 1 = GLFW_PRESS
    
    if (key == 256) { // ESC
        hide();
        return true;
    }
    
    if (key == 257) { // Enter
        applySettings();
        hide();
        if (onClose) onClose();
        return true;
    }
    
    // Arrow keys for font selection
    if (key == 264) { // Down arrow
        if (selectedFontIndex_ < static_cast<int>(availableFonts_.size()) - 1) {
            selectFont(selectedFontIndex_ + 1);
        }
        return true;
    }
    
    if (key == 265) { // Up arrow
        if (selectedFontIndex_ > 0) {
            selectFont(selectedFontIndex_ - 1);
        }
        return true;
    }
    
    return false;
}

void SettingsUI::selectFont(int index) {
    if (index < 0 || index >= static_cast<int>(availableFonts_.size())) return;
    
    // Deselect previous
    if (selectedFontIndex_ >= 0 && selectedFontIndex_ < static_cast<int>(availableFonts_.size())) {
        availableFonts_[selectedFontIndex_].isSelected = false;
    }
    
    selectedFontIndex_ = index;
    availableFonts_[index].isSelected = true;
    
    // Scroll to show selected item
    float visibleItems = (fontListEndY_ - fontListStartY_) / itemHeight_;
    if (index < scrollOffset_) {
        scrollOffset_ = index;
    } else if (index >= scrollOffset_ + static_cast<int>(visibleItems)) {
        scrollOffset_ = index - static_cast<int>(visibleItems) + 1;
    }
}

void SettingsUI::applySettings() {
    if (selectedFontIndex_ >= 0 && selectedFontIndex_ < static_cast<int>(availableFonts_.size())) {
        settings_->setString("font.path", availableFonts_[selectedFontIndex_].path);
    }
    
    if (fontSizeChanged_) {
        settings_->setInt("font.size", fontSize_);
    }

    // Save settings
    const char* homeDir = getenv("HOME");
    std::string configPath;
    if (homeDir) {
        configPath = std::string(homeDir) + "/.hyperterm/config";
    } else {
        std::cerr << "Warning: HOME environment variable not set. Using current directory for config." << std::endl;
        configPath = "./.hyperterm/config";
    }
    settings_->save(configPath);
}

bool SettingsUI::isPointInRect(float x, float y, float rectX, float rectY, float rectW, float rectH) {
    return x >= rectX && x <= rectX + rectW && y >= rectY && y <= rectY + rectH;
}

