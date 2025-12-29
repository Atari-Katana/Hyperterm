#pragma once

#include <string>
#include <vector>
#include <memory>
#include <cstdint>
#include <functional>
#include <vulkan/vulkan.h>
#include <deque>
#include "settings/settings.hpp"
class VulkanRenderer; // Forward declaration


const size_t MAX_SCROLLBACK_LINES = 1000;

struct Cell {
    char32_t character;
    uint32_t fgColor;  // RGB
    uint32_t bgColor;  // RGB
    bool bold;
    bool underline;
    
    Cell() : character(' '), fgColor(0xFFFFFF), bgColor(0x000000), bold(false), underline(false) {}
};

class TerminalSession {
public:
    TerminalSession(uint32_t rows, uint32_t cols, VulkanRenderer* renderer, const ColorScheme* colorScheme);
    ~TerminalSession();
    
    bool startShell();
    void stopShell();
    
    void writeInput(const std::string& data);
    void processOutput(const std::string& data);
    
    void resize(uint32_t rows, uint32_t cols);
    
    // Public getters now return current active buffer's state
    const std::vector<std::vector<Cell>>& getCells() const { return useAlternateBuffer_ ? altCells_ : cells_; }
    const std::deque<std::vector<Cell>>& getScrollback() const { return scrollback_; } // Scrollback is shared
    size_t getScrollbackSize() const { return scrollback_.size(); }
    uint32_t getRows() const { return rows_; }
    uint32_t getCols() const { return cols_; }
    uint32_t getCursorRow() const { return useAlternateBuffer_ ? altCursorRow_ : cursorRow_; }
    uint32_t getCursorCol() const { return useAlternateBuffer_ ? altCursorCol_ : cursorCol_; }
    
    void setBackgroundImage(const std::string& path);
    const std::string& getBackgroundImage() const { return backgroundImage_; }
    VkImageView getBackgroundImage_() const { return backgroundImageTextureView_; }
    
    int getMasterFd() const { return masterFd_; }
    
    std::function<void()> onOutput;
    
private:
    uint32_t rows_;
    uint32_t cols_;

    // Main screen buffer
    std::vector<std::vector<Cell>> cells_;
    std::deque<std::vector<Cell>> scrollback_;
    uint32_t cursorRow_;
    uint32_t cursorCol_;

    // Alternate screen buffer
    std::vector<std::vector<Cell>> altCells_;
    uint32_t altCursorRow_;
    uint32_t altCursorCol_;
    bool useAlternateBuffer_;
    
    int masterFd_;
    int slaveFd_;
    pid_t shellPid_;
    
    VulkanRenderer* renderer_; // Restored
    const ColorScheme* colorScheme_;
    std::string backgroundImage_;
    VkImage backgroundImageTexture_;
    VkDeviceMemory backgroundImageTextureMemory_;
    VkImageView backgroundImageTextureView_;
    
    uint32_t currentFgColor_;
    uint32_t currentBgColor_;
    bool currentBold_;
    bool currentUnderline_;
    std::string escapeBuffer_;
    
    // For UTF-8 decoding
    uint32_t utf8_state_ = 0;
    uint32_t utf8_codepoint_ = 0;
    
    void destroyBackgroundImage();
    void parseEscapeSequence(const std::string& sequence);
    void parseCSI(const std::string& params);
    uint32_t parseColorCode(int code);
    void clearScreen(); // Operates on active buffer
    void moveCursor(uint32_t row, uint32_t col); // Operates on active buffer
    void setForegroundColor(uint32_t color);
    void setBackgroundColor(uint32_t color);
    void putChar(char32_t c); // Operates on active buffer
    void newLine(); // Operates on active buffer
    void backspace(); // Operates on active buffer
    void processByte(unsigned char byte);

    // Helper to get a reference to the currently active cells and cursor
    std::vector<std::vector<Cell>>& getActiveCells();
    uint32_t& getActiveCursorRow();
    uint32_t& getActiveCursorCol();
