#pragma once

#include "terminal_session.hpp"
#include "settings/settings.hpp" // Include for ColorScheme
#include <vector>
#include <memory>
#include <cstdint>

class VulkanRenderer;

class TerminalManager {
public:
    TerminalManager(uint32_t rows, uint32_t cols);
    
    size_t createSession(VulkanRenderer* renderer, const ColorScheme* colorScheme);
    void destroySession(size_t index);
    TerminalSession* getSession(size_t index);
    size_t getActiveSessionIndex() const { return activeSessionIndex_; }
    void setActiveSession(size_t index);
    
    size_t getSessionCount() const { return sessions_.size(); }
    
    void update();
    
private:
    std::vector<std::unique_ptr<TerminalSession>> sessions_;
    size_t activeSessionIndex_;
    uint32_t defaultRows_;
    uint32_t defaultCols_;
};

