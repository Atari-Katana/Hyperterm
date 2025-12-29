#include "settings.hpp"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <iostream>
#include <stdexcept>

namespace {
    uint32_t hexToUint32(const std::string& hex) {
        if (hex.empty()) return 0;
        std::string s = hex;
        if (s[0] == '#') s.erase(0, 1);
        if (s.rfind("0x", 0) == 0) s.erase(0, 2);
        
        try {
            return std::stoul(s, nullptr, 16);
        } catch (...) {
            return 0; // Default to black on error
        }
    }
}

Settings::Settings() {
    // Set defaults
    setInt("font.size", 16);
    setString("font.path", "fonts/default.ttf");
    setString("background.default", "");
}

Settings::~Settings() {
}

bool Settings::load(const std::string& configPath) {
    configPath_ = configPath;
    std::ifstream file(configPath);
    if (!file.is_open()) {
        return false;
    }
    
    std::string line;
    while (std::getline(file, line)) {
        size_t eqPos = line.find('=');
        if (eqPos != std::string::npos) {
            std::string key = line.substr(0, eqPos);
            std::string value = line.substr(eqPos + 1);
            // Trim whitespace
            key.erase(0, key.find_first_not_of(" 	"));
            key.erase(key.find_last_not_of(" 	") + 1);
            value.erase(0, value.find_first_not_of(" 	"));
            value.erase(value.find_last_not_of(" 	") + 1);
            values_[key] = value; // Assign the value here
        }
    }
    
    // Load color scheme settings
    currentColorScheme_ = ColorScheme(); // Reset to defaults
    
    if (values_.count("color.defaultFg")) {
        currentColorScheme_.defaultFg = hexToUint32(values_["color.defaultFg"]);
    }
    if (values_.count("color.defaultBg")) {
        currentColorScheme_.defaultBg = hexToUint32(values_["color.defaultBg"]);
    }
    for (int i = 0; i < 16; ++i) {
        std::string key = "color.ansi" + std::to_string(i);
        if (values_.count(key)) {
            currentColorScheme_.ansiColors[i] = hexToUint32(values_[key]);
        }
    }
    
    return true;
}

bool Settings::save(const std::string& configPath) {
    std::ofstream file(configPath);
    if (!file.is_open()) {
        return false;
    }
    
    for (const auto& pair : values_) {
        file << pair.first << "=" << pair.second << "\n";
    }
    
    return true;
}

std::string Settings::getString(const std::string& key, const std::string& defaultValue) const {
    auto it = values_.find(key);
    if (it != values_.end()) {
        return it->second;
    }
    return defaultValue;
}

int Settings::getInt(const std::string& key, int defaultValue) const {
    auto it = values_.find(key);
    if (it != values_.end()) {
        try {
            return std::stoi(it->second);
        } catch (const std::invalid_argument& e) {
            std::cerr << "Warning: invalid integer value for key '" << key << "'. Using default." << std::endl;
        } catch (const std::out_of_range& e) {
            std::cerr << "Warning: integer value for key '" << key << "' out of range. Using default." << std::endl;
        }
    }
    return defaultValue;
}

float Settings::getFloat(const std::string& key, float defaultValue) const {
    auto it = values_.find(key);
    if (it != values_.end()) {
        try {
            return std::stof(it->second);
        } catch (const std::invalid_argument& e) {
            std::cerr << "Warning: invalid float value for key '" << key << "'. Using default." << std::endl;
        } catch (const std::out_of_range& e) {
            std::cerr << "Warning: float value for key '" << key << "' out of range. Using default." << std::endl;
        }
    }
    return defaultValue;
}

bool Settings::getBool(const std::string& key, bool defaultValue) const {
    auto it = values_.find(key);
    if (it != values_.end()) {
        return it->second == "true" || it->second == "1";
    }
    return defaultValue;
}

void Settings::setString(const std::string& key, const std::string& value) {
    values_[key] = value;
}

void Settings::setInt(const std::string& key, int value) {
    values_[key] = std::to_string(value);
}

void Settings::setFloat(const std::string& key, float value) {
    values_[key] = std::to_string(value);
}

void Settings::setBool(const std::string& key, bool value) {
    values_[key] = value ? "true" : "false";
}