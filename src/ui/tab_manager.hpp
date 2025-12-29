#pragma once

#include <vector>
#include <string>
#include <cstdint>

class TabManager {
public:
    struct Tab {
        size_t terminalIndex;
        std::string title;
        bool isActive;
        
        Tab(size_t idx) : terminalIndex(idx), title("Terminal"), isActive(false) {}
    };
    
    TabManager();
    
    size_t createTab(size_t terminalIndex);
    void closeTab(size_t index);
    void setActiveTab(size_t index);
    size_t getActiveTabIndex() const { return activeTabIndex_; }
    
    Tab* getTab(size_t index);
    size_t getTabCount() const { return tabs_.size(); }
    
    void updateTabTitle(size_t index, const std::string& title);
    
private:
    std::vector<Tab> tabs_;
    size_t activeTabIndex_;
};

