#include "terminal_manager.hpp"
#include "terminal_session.hpp"
#include <algorithm>
#include <unistd.h>
#include <sys/select.h>

TerminalManager::TerminalManager(uint32_t rows, uint32_t cols)
    : activeSessionIndex_(0), defaultRows_(rows), defaultCols_(cols) {
}

size_t TerminalManager::createSession(VulkanRenderer* renderer, const ColorScheme* colorScheme) {
    auto session = std::make_unique<TerminalSession>(defaultRows_, defaultCols_, renderer, colorScheme);
    if (session->startShell()) {
        sessions_.push_back(std::move(session));
        activeSessionIndex_ = sessions_.size() - 1;
        return activeSessionIndex_;
    }
    return SIZE_MAX;
}

void TerminalManager::destroySession(size_t index) {
    if (index < sessions_.size()) {
        sessions_.erase(sessions_.begin() + index);
        if (activeSessionIndex_ >= sessions_.size() && !sessions_.empty()) {
            activeSessionIndex_ = sessions_.size() - 1;
        }
        if (sessions_.empty()) {
            activeSessionIndex_ = 0;
        }
    }
}

TerminalSession* TerminalManager::getSession(size_t index) {
    if (index < sessions_.size()) {
        return sessions_[index].get();
    }
    return nullptr;
}

void TerminalManager::setActiveSession(size_t index) {
    if (index < sessions_.size()) {
        activeSessionIndex_ = index;
    }
}

void TerminalManager::update() {
    // Poll PTY file descriptors and process output
    fd_set readfds;
    FD_ZERO(&readfds);
    int maxFd = -1;
    
    for (auto& session : sessions_) {
        if (session) {
            int fd = session->getMasterFd();
            if (fd >= 0) {
                FD_SET(fd, &readfds);
                if (fd > maxFd) maxFd = fd;
            }
        }
    }
    
    if (maxFd >= 0) {
        struct timeval timeout;
        timeout.tv_sec = 0;
        timeout.tv_usec = 0;
        
        if (select(maxFd + 1, &readfds, nullptr, nullptr, &timeout) > 0) {
            for (size_t i = 0; i < sessions_.size(); i++) {
                if (sessions_[i]) {
                    int fd = sessions_[i]->getMasterFd();
                    if (fd >= 0 && FD_ISSET(fd, &readfds)) {
                        char buffer[4096];
                        int bytesRead = read(fd, buffer, sizeof(buffer));
                        if (bytesRead > 0) {
                            sessions_[i]->processOutput(std::string(buffer, bytesRead));
                        }
                    }
                }
            }
        }
    }
}

