#pragma once

#include <string>
#include <map>
#include <cstdint>
#include <array>

struct ColorScheme {
    uint32_t defaultFg;
    uint32_t defaultBg;
    std::array<uint32_t, 16> ansiColors; // ANSI 0-15
    
    ColorScheme() : defaultFg(0xFFFFFF), defaultBg(0x000000) {
        // Default ANSI colors (xterm-256 color palette defaults)
        ansiColors = {
            0x000000, 0xAA0000, 0x00AA00, 0xAA5500, 0x0000AA, 0xAA00AA, 0x00AAAA, 0xAAAAAA,
            0x555555, 0xFF5555, 0x55FF55, 0xFFFF55, 0x5555FF, 0xFF55FF, 0x55FFFF, 0xFFFFFF
        };
    }
};

class Settings {
public:
    Settings();
    ~Settings();
    
    bool load(const std::string& configPath);
    bool save(const std::string& configPath);
    
    std::string getString(const std::string& key, const std::string& defaultValue = "") const;
    int getInt(const std::string& key, int defaultValue = 0) const;
    float getFloat(const std::string& key, float defaultValue = 0.0f) const;
    bool getBool(const std::string& key, bool defaultValue = false) const;
    
    void setString(const std::string& key, const std::string& value);
    void setInt(const std::string& key, int value);
    void setFloat(const std::string& key, float value);
    void setBool(const std::string& key, bool value);
    
    std::string getFontPath() const { return getString("font.path", "fonts/default.ttf"); }
    uint32_t getFontSize() const { return static_cast<uint32_t>(getInt("font.size", 16)); }
    std::string getDefaultBackground() const { return getString("background.default", ""); }
    
    const ColorScheme& getCurrentColorScheme() const { return currentColorScheme_; }
    
private:
    std::map<std::string, std::string> values_;
    std::string configPath_;
    ColorScheme currentColorScheme_;
};

