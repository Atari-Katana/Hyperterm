#include "terminal_session.hpp"
#include "renderer/vulkan_renderer.hpp"
#include "renderer/stb_image.h"
#include <unistd.h>
#include <sys/wait.h>
#include <sys/ioctl.h>
#include <pty.h>
#include <termios.h>
#include <fcntl.h>
#include <signal.h>
#include <stdexcept>
#include <algorithm>
#include <sstream>
#include <vector>
#include <string>
#include <iostream>

TerminalSession::TerminalSession(uint32_t rows, uint32_t cols, VulkanRenderer* renderer, const ColorScheme* colorScheme)
    : rows_(rows), cols_(cols), 
      cursorRow_(0), cursorCol_(0), 
      altCursorRow_(0), altCursorCol_(0), 
      useAlternateBuffer_(false), // Initialize alternate buffer usage
      masterFd_(-1), slaveFd_(-1), shellPid_(-1),
      renderer_(renderer), colorScheme_(colorScheme),
      backgroundImageTexture_(VK_NULL_HANDLE), backgroundImageTextureMemory_(VK_NULL_HANDLE), backgroundImageTextureView_(VK_NULL_HANDLE),
      currentFgColor_(colorScheme->defaultFg), currentBgColor_(colorScheme->defaultBg), currentBold_(false), currentUnderline_(false),
      utf8_state_(0), utf8_codepoint_(0) {
    cells_.resize(rows_);
    for (auto& row : cells_) {
        row.resize(cols_);
    }
    altCells_.resize(rows_);
    for (auto& row : altCells_) {
        row.resize(cols_);
    }
}

TerminalSession::~TerminalSession() {
    destroyBackgroundImage();
    stopShell();
}

bool TerminalSession::startShell() {
    struct winsize ws;
    ws.ws_row = rows_;
    ws.ws_col = cols_;
    ws.ws_xpixel = 0;
    ws.ws_ypixel = 0;
    
    if (openpty(&masterFd_, &slaveFd_, nullptr, nullptr, &ws) == -1) {
        return false;
    }
    
    shellPid_ = fork();
    if (shellPid_ == 0) {
        // Child process
        close(masterFd_);

        setsid();
        if (ioctl(slaveFd_, TIOCSCTTY, nullptr) == -1) {
            _exit(1);
        }

        dup2(slaveFd_, STDIN_FILENO);
        dup2(slaveFd_, STDOUT_FILENO);
        dup2(slaveFd_, STDERR_FILENO);

        if (slaveFd_ > STDERR_FILENO) {
            close(slaveFd_);
        }

        const char* shell_env = getenv("SHELL");
        const char* shell = "/bin/bash"; // default shell
        if (shell_env) {
            const std::vector<std::string> allowed_shells = {
                "/bin/bash", "/bin/sh", "/bin/zsh", "/bin/fish",
                "/usr/bin/bash", "/usr/bin/sh", "/usr/bin/zsh", "/usr/bin/fish"
            };
            bool allowed = false;
            for (const auto& s : allowed_shells) {
                if (s == shell_env) {
                    shell = shell_env;
                    allowed = true;
                    break;
                }
            }
            if (!allowed) {
                std::cerr << "Warning: SHELL environment variable ('" << shell_env << "') is not in the allow-list. Falling back to default shell." << std::endl;
            }
        }

        execl(shell, shell, nullptr);
        _exit(1);
    } else if (shellPid_ > 0) {
        // Parent process - success
        close(slaveFd_);
        fcntl(masterFd_, F_SETFL, O_NONBLOCK);
        return true;
    } else {
        // Fork failed - clean up file descriptors
        std::cerr << "Error: fork() failed" << std::endl;
        close(masterFd_);
        close(slaveFd_);
        masterFd_ = -1;
        slaveFd_ = -1;
        return false;
    }

    return false;
}

void TerminalSession::stopShell() {
    if (shellPid_ > 0) {
        kill(shellPid_, SIGTERM);
        waitpid(shellPid_, nullptr, 0);
        shellPid_ = -1;
    }
    if (masterFd_ >= 0) {
        close(masterFd_);
        masterFd_ = -1;
    }
}

void TerminalSession::writeInput(const std::string& data) {
    if (masterFd_ >= 0) {
        write(masterFd_, data.c_str(), data.length());
    }
}

void TerminalSession::processOutput(const std::string& data) {
    for (char c : data) {
        processByte(static_cast<unsigned char>(c));
    }
    
    if (onOutput) {
        onOutput();
    }
}

// UTF-8 decoder based on https://www.cl.cam.ac.uk/~mgk25/ucs/utf-8-history.txt
// Copyright (c) 2008-2009 Bjoern Hoehrmann <bjoern@hoehrmann.de>
#define UTF8_ACCEPT 0
#define UTF8_REJECT 1

static const uint8_t utf8d[] = {
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, // 00..1f
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, // 20..3f
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, // 40..5f
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, // 60..7f
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9, // 80..9f
  7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7, // a0..bf
  8,8,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2, // c0..df
  0xa,0x3,0x3,0x3,0x3,0x3,0x3,0x3,0x3,0x3,0x3,0x3,0x3,0x4,0x3,0x3, // e0..ef
  0xb,0x6,0x6,0x6,0x5,0x8,0x8,0x8,0x8,0x8,0x8,0x8,0x8,0x8,0x8,0x8, // f0..ff
  0x0,0x1,0x2,0x3,0x5,0x8,0x7,0x1,0x1,0x1,0x4,0x6,0x1,0x1,0x1,0x1, // s0..s0
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,1,1,1,1,1,0,1,0,1,1,1,1,1,1, // s1..s2
  1,2,1,1,1,1,1,2,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1, // s3..s4
  1,2,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,3,1,3,1,1,1,1,1,1, // s5..s6
  1,3,1,1,1,1,1,3,1,3,1,1,1,1,1,1,1,3,1,1,1,1,1,1,1,1,1,1,1,1,1,1, // s7..s8
};

uint32_t decode(uint32_t* state, uint32_t* codep, uint32_t byte) {
  uint32_t type = utf8d[byte];
  *codep = (*state != UTF8_ACCEPT) ?
    (byte & 0x3fu) | (*codep << 6) :
    (0xff >> type) & (byte);
  *state = utf8d[256 + *state*16 + type];
  return *state;
}

void TerminalSession::processByte(unsigned char byte) {
    if (!escapeBuffer_.empty()) {
        escapeBuffer_ += byte;
        if ((byte >= 0x40 && byte <= 0x7E) || escapeBuffer_.length() > 50) {
            parseEscapeSequence(escapeBuffer_);
            escapeBuffer_.clear();
        }
        return;
    }

    if (byte < 0x20 || byte == 0x7F) { // C0 controls
        switch (byte) {
        case '\n': newLine(); break;
        case '\r': getActiveCursorCol() = 0; break; // Use active cursor
        case '\b': backspace(); break;
        case '\t': 
        {
            uint32_t& currentCursorCol = getActiveCursorCol();
            for (int i = 0; i < 8 - (static_cast<int>(currentCursorCol) % 8); ++i) putChar(' '); 
        }
        break;
        case '\x1b': escapeBuffer_ = "\x1b"; break;
        case 0x7F: backspace(); break; // DEL
        // Other C0 controls are ignored
        }
        return;
    }
    
    // Decode UTF-8
    if (decode(&utf8_state_, &utf8_codepoint_, byte) == UTF8_ACCEPT) {
        putChar(utf8_codepoint_);
    } else if (utf8_state_ == UTF8_REJECT) {
        utf8_state_ = UTF8_ACCEPT; // Reset on error
    }
}

void TerminalSession::resize(uint32_t rows, uint32_t cols) {
    rows_ = rows;
    cols_ = cols;
    
    cells_.resize(rows_);
    for (auto& row : cells_) {
        row.resize(cols_);
    }
    
    altCells_.resize(rows_);
    for (auto& row : altCells_) {
        row.resize(cols_);
    }

    if (cursorRow_ >= rows_) cursorRow_ = rows_ - 1;
    if (cursorCol_ >= cols_) cursorCol_ = cols_ - 1;
    if (altCursorRow_ >= rows_) altCursorRow_ = rows_ - 1;
    if (altCursorCol_ >= cols_) altCursorCol_ = cols_ - 1;
    
    if (masterFd_ >= 0) {
        struct winsize ws;
        ws.ws_row = rows_;
        ws.ws_col = cols_;
        ioctl(masterFd_, TIOCSWINSZ, &ws);
    }
}

void TerminalSession::setBackgroundImage(const std::string& path) {
    backgroundImage_ = path;
    
    destroyBackgroundImage();
    
    if (!backgroundImage_.empty() && renderer_) {
        int imgWidth, imgHeight, channels;
        unsigned char* data = stbi_load(backgroundImage_.c_str(), &imgWidth, &imgHeight, &channels, 4);
        if (data) {
            renderer_->createTexture(imgWidth, imgHeight, data, backgroundImageTexture_, backgroundImageTextureMemory_, backgroundImageTextureView_);
            stbi_image_free(data);
        }
    }
}

std::vector<std::vector<Cell>>& TerminalSession::getActiveCells() {
    return useAlternateBuffer_ ? altCells_ : cells_;
}

uint32_t& TerminalSession::getActiveCursorRow() {
    return useAlternateBuffer_ ? altCursorRow_ : cursorRow_;
}

uint32_t& TerminalSession::getActiveCursorCol() {
    return useAlternateBuffer_ ? altCursorCol_ : cursorCol_;
}

void TerminalSession::putChar(char32_t c) {
    std::vector<std::vector<Cell>>& currentCells = getActiveCells();
    uint32_t& currentCursorRow = getActiveCursorRow();
    uint32_t& currentCursorCol = getActiveCursorCol();

    if (currentCursorRow < rows_ && currentCursorCol < cols_) {
        currentCells[currentCursorRow][currentCursorCol].character = c;
        currentCells[currentCursorRow][currentCursorCol].fgColor = currentFgColor_;
        currentCells[currentCursorRow][currentCursorCol].bgColor = currentBgColor_;
        currentCells[currentCursorRow][currentCursorCol].bold = currentBold_;
        currentCells[currentCursorRow][currentCursorCol].underline = currentUnderline_;

        currentCursorCol++;
        if (currentCursorCol >= cols_) {
            newLine();
        }
    }
}

void TerminalSession::newLine() {
    uint32_t& currentCursorRow = getActiveCursorRow();
    uint32_t& currentCursorCol = getActiveCursorCol();
    std::vector<std::vector<Cell>>& currentCells = getActiveCells();

    currentCursorRow++;
    currentCursorCol = 0;
    if (currentCursorRow >= rows_) {
        // If not in alternate buffer, add to scrollback
        if (!useAlternateBuffer_) {
            if (scrollback_.size() >= MAX_SCROLLBACK_LINES) {
                scrollback_.pop_front();
            }
            scrollback_.push_back(currentCells.front());
        }
        
        // Scroll up
        currentCells.erase(currentCells.begin());
        currentCells.push_back(std::vector<Cell>(cols_));
        currentCursorRow = rows_ - 1;
    }
}

void TerminalSession::backspace() {
    uint32_t& currentCursorRow = getActiveCursorRow();
    uint32_t& currentCursorCol = getActiveCursorCol();
    std::vector<std::vector<Cell>>& currentCells = getActiveCells();

    if (currentCursorCol > 0) {
        currentCursorCol--;
        currentCells[currentCursorRow][currentCursorCol] = Cell();
    }
}

void TerminalSession::destroyBackgroundImage() {
    if (renderer_ && backgroundImageTextureView_ != VK_NULL_HANDLE) {
        renderer_->destroyTexture(backgroundImageTexture_, backgroundImageTextureMemory_, backgroundImageTextureView_);
        backgroundImageTexture_ = VK_NULL_HANDLE;
        backgroundImageTextureMemory_ = VK_NULL_HANDLE;
        backgroundImageTextureView_ = VK_NULL_HANDLE;
    }
}

// Helper to parse color from SGR parameters
// codes: SGR parameter list
// i: current index in codes, will be advanced past the color parameters
// colorScheme: current color scheme to use for ANSI 0-15
uint32_t parseSGRColor(const std::vector<int>& codes, size_t& i, const ColorScheme* colorScheme) {
    if (i + 1 < codes.size()) {
        if (codes[i + 1] == 5) { // 256-color: ESC[38;5;N m (or 48)
            if (i + 2 < codes.size()) {
                i += 2; // Advance past 5 and the 'N'
                int color_idx = codes[i];
                // XTerm 256 color palette mapping
                if (color_idx < 16) { // Standard 16 colors
                    return colorScheme->ansiColors[color_idx];
                } else if (color_idx >= 16 && color_idx <= 231) { // 6x6x6 color cube
                    int r_idx = (color_idx - 16) / 36;
                    int g_idx = ((color_idx - 16) % 36) / 6;
                    int b_idx = (color_idx - 16) % 6;
                    uint8_t r = (r_idx == 0) ? 0 : r_idx * 40 + 55;
                    uint8_t g = (g_idx == 0) ? 0 : g_idx * 40 + 55;
                    uint8_t b = (b_idx == 0) ? 0 : b_idx * 40 + 55;
                    return (r << 16) | (g << 8) | b;
                } else if (color_idx >= 232 && color_idx <= 255) { // Grayscale
                    uint8_t gray = 8 + (color_idx - 232) * 10;
                    return (gray << 16) | (gray << 8) | gray;
                }
            }
        } else if (codes[i + 1] == 2) { // True-color: ESC[38;2;R;G;B m (or 48)
            if (i + 4 < codes.size()) {
                i += 4; // Advance past 2 and R;G;B
                uint32_t r = std::clamp(codes[i - 2], 0, 255);
                uint32_t g = std::clamp(codes[i - 1], 0, 255);
                uint32_t b = std::clamp(codes[i], 0, 255);
                return (r << 16) | (g << 8) | b;
            }
        }
    }
    return colorScheme->defaultFg; // Fallback to default foreground
}


void TerminalSession::parseEscapeSequence(const std::string& sequence) {
    if (sequence.empty()) return; 
    
    if (sequence[0] == '\x1b') {
        if (sequence.length() >= 2 && sequence[1] == '[') {
            // CSI sequence: ESC [
            std::string params = sequence.substr(2);
            parseCSI(params);
        } else if (sequence.length() >= 2 && sequence[1] == 'c') {
            // Reset terminal: ESC c
            clearScreen();
            currentFgColor_ = colorScheme_->defaultFg; // Use scheme default
            currentBgColor_ = colorScheme_->defaultBg; // Use scheme default
            currentBold_ = false;
            currentUnderline_ = false;
        } else if (sequence.length() >= 2 && sequence[1] == '>') {
            // DECPrivateMode: ESC >
            // Currently ignored
        }
    }
}

void TerminalSession::parseCSI(const std::string& params) {
    if (params.empty()) {
        // Empty CSI - treat as cursor move to home
        moveCursor(0, 0);
        return;
    }
    
    // Find the command character (last character)
    char cmd = params.back();
    std::string nums = params.substr(0, params.length() - 1);
    
    std::istringstream iss(nums);
    std::vector<int> codes;
    std::string codeStr;
    
    // Parse semicolon-separated codes
    while (std::getline(iss, codeStr, ';')) {
        if (codes.size() >= 64) {
            // Safety: prevent DoS from too many codes
            break;
        }
        if (!codeStr.empty()) {
            try {
                codes.push_back(std::stoi(codeStr));
            } catch (...) {
                codes.push_back(0);
            }
        } else {
            codes.push_back(0);
        }
    }
    
    if (codes.empty()) codes.push_back(0);
    
    // Handle different CSI commands based on final character
    if (cmd == 'm') {
        // SGR (Select Graphic Rendition) - colors and styles
        for (size_t i = 0; i < codes.size(); ++i) { // Use size_t for index
            int c = codes[i];
            if (c == 0) {
                // Reset attributes
                currentFgColor_ = colorScheme_->defaultFg; // Use scheme default
                currentBgColor_ = colorScheme_->defaultBg; // Use scheme default
                currentBold_ = false;
                currentUnderline_ = false;
            } else if (c >= 30 && c <= 37) {
                // Set foreground color (30-37: standard colors)
                currentFgColor_ = parseColorCode(c - 30);
            } else if (c == 39) {
                // Default foreground color
                currentFgColor_ = colorScheme_->defaultFg; // Use scheme default
            } else if (c >= 40 && c <= 47) {
                // Set background color (40-47: standard colors)
                currentBgColor_ = parseColorCode(c - 40);
            } else if (c == 49) {
                // Default background color
                currentBgColor_ = colorScheme_->defaultBg; // Use scheme default
            } else if (c == 1) {
                // Bold
                currentBold_ = true;
            } else if (c == 4) {
                // Underline
                currentUnderline_ = true;
            } else if (c == 22) {
                // Normal intensity (not bold)
                currentBold_ = false;
            } else if (c == 24) {
                // Not underlined
                currentUnderline_ = false;
            } else if (c == 38) { // Set foreground color extended (256-color or true-color)
                currentFgColor_ = parseSGRColor(codes, i, colorScheme_);
            } else if (c == 48) { // Set background color extended (256-color or true-color)
                currentBgColor_ = parseSGRColor(codes, i, colorScheme_);
            }
        }
    } else if (cmd == 'J') {
        // Erase display
        if (codes[0] == 2) { // Use codes[0] instead of 'code'
            clearScreen();
        }
    } else if (cmd == 'H' || cmd == 'f') {
        // Cursor Position
        uint32_t row = codes.size() >= 1 ? std::max(0, codes[0] - 1) : 0;
        uint32_t col = codes.size() >= 2 ? std::max(0, codes[1] - 1) : 0;
        moveCursor(row, col);
    } else if (cmd == 'A') {
        // Cursor Up: ESC [nA
        int n = codes[0] > 0 ? codes[0] : 1; // Use codes[0] instead of 'code'
        getActiveCursorRow() = (getActiveCursorRow() >= static_cast<uint32_t>(n)) ? getActiveCursorRow() - n : 0;
    } else if (cmd == 'B') {
        // Cursor Down: ESC [nB
        int n = codes[0] > 0 ? codes[0] : 1; // Use codes[0] instead of 'code'
        getActiveCursorRow() = std::min(getActiveCursorRow() + static_cast<uint32_t>(n), rows_ - 1);
    } else if (cmd == 'C') {
        // Cursor Forward: ESC [nC
        int n = codes[0] > 0 ? codes[0] : 1; // Use codes[0] instead of 'code'
        getActiveCursorCol() = std::min(getActiveCursorCol() + static_cast<uint32_t>(n), cols_ - 1);
    } else if (cmd == 'D') {
        // Cursor Back: ESC [nD
        int n = codes[0] > 0 ? codes[0] : 1; // Use codes[0] instead of 'code'
        getActiveCursorCol() = (getActiveCursorCol() >= static_cast<uint32_t>(n)) ? getActiveCursorCol() - n : 0;
    } else if (cmd == 'K') {
        // Erase in line
        int code = codes[0];
        uint32_t& currentCursorCol = getActiveCursorCol();
        std::vector<std::vector<Cell>>& currentCells = getActiveCells();

        if (code == 0) { // Erase from cursor to end
            for (uint32_t col = currentCursorCol; col < cols_; col++) {
                currentCells[getActiveCursorRow()][col] = Cell();
            }
        } else if (code == 1) { // Erase from beginning to cursor
            for (uint32_t col = 0; col <= currentCursorCol; col++) {
                currentCells[getActiveCursorRow()][col] = Cell();
            }
        } else if (code == 2) { // Erase entire line
            for (uint32_t col = 0; col < cols_; col++) {
                currentCells[getActiveCursorRow()][col] = Cell();
            }
        }
    } else if (cmd == 'h' || cmd == 'l') { // Set Mode / Reset Mode
        if (nums == "?1049") { // Alternate Screen Buffer
            if (cmd == 'h') { // Enable alternate buffer
                useAlternateBuffer_ = true;
                altCells_ = std::vector<std::vector<Cell>>(rows_, std::vector<Cell>(cols_));
                altCursorRow_ = 0;
                altCursorCol_ = 0;
                clearScreen(); // Clear alt screen
            } else { // Disable alternate buffer
                useAlternateBuffer_ = false;
            }
        }
    }
}

uint32_t TerminalSession::parseColorCode(int code) {
    if (code >= 0 && code < 16) { // Use ANSI 0-15 from color scheme
        return colorScheme_->ansiColors[code];
    }
    return colorScheme_->defaultFg; // Default to foreground color from scheme
}

void TerminalSession::clearScreen() {
    std::vector<std::vector<Cell>>& currentCells = getActiveCells();
    uint32_t& currentCursorRow = getActiveCursorRow();
    uint32_t& currentCursorCol = getActiveCursorCol();

    for (auto& row : currentCells) {
        for (auto& cell : row) {
            cell = Cell();
        }
    }
    // Only clear scrollback if not in alternate buffer
    if (!useAlternateBuffer_) {
        scrollback_.clear();
    }
    currentCursorRow = 0;
    currentCursorCol = 0;
}

void TerminalSession::moveCursor(uint32_t row, uint32_t col) {
    uint32_t& currentCursorRow = getActiveCursorRow();
    uint32_t& currentCursorCol = getActiveCursorCol();

    currentCursorRow = std::min(row, rows_ - 1);
    currentCursorCol = std::min(col, cols_ - 1);
}

void TerminalSession::setForegroundColor(uint32_t color) {
    currentFgColor_ = color;
}

void TerminalSession::setBackgroundColor(uint32_t color) {
    currentBgColor_ = color;
}