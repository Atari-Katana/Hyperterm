#include "tab_manager.hpp"
#include <algorithm>

TabManager::TabManager() : activeTabIndex_(0) {
}

size_t TabManager::createTab(size_t terminalIndex) {
    tabs_.emplace_back(terminalIndex);
    size_t newIndex = tabs_.size() - 1;
    setActiveTab(newIndex);
    return newIndex;
}

void TabManager::closeTab(size_t index) {
    if (index < tabs_.size()) {
        tabs_.erase(tabs_.begin() + index);
        if (activeTabIndex_ >= tabs_.size() && !tabs_.empty()) {
            activeTabIndex_ = tabs_.size() - 1;
        }
        if (tabs_.empty()) {
            activeTabIndex_ = 0;
        } else if (activeTabIndex_ > index) {
            activeTabIndex_--;
        }
    }
}

void TabManager::setActiveTab(size_t index) {
    if (index < tabs_.size()) {
        for (auto& tab : tabs_) {
            tab.isActive = false;
        }
        tabs_[index].isActive = true;
        activeTabIndex_ = index;
    }
}

TabManager::Tab* TabManager::getTab(size_t index) {
    if (index < tabs_.size()) {
        return &tabs_[index];
    }
    return nullptr;
}

void TabManager::updateTabTitle(size_t index, const std::string& title) {
    if (index < tabs_.size()) {
        tabs_[index].title = title;
    }
}

