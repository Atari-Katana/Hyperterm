#include "pane_manager.hpp"
#include "../application.hpp" // For Application::drawTerminalContent
#include "../renderer/vulkan_renderer.hpp"
#include "../settings/settings.hpp"
#include <iostream>
#include <unistd.h>
#include <sys/select.h>

PaneManager::PaneManager(Application* app, VulkanRenderer* renderer, Settings* settings)
    : app_(app), renderer_(renderer), settings_(settings), activePane_(nullptr), nextPaneId_(0) {
}

PaneManager::~PaneManager() {
    // All panes will be deallocated automatically by unique_ptr
}

Pane* PaneManager::createRootPane() {
    auto newPane = std::make_unique<Pane>();
    newPane->id = nextPaneId_++;
    newPane->session = std::make_unique<TerminalSession>(80, 24, renderer_, &settings_->getCurrentColorScheme());
    
    rootPanes_.push_back(std::move(newPane));
    
    // Set as active if it's the first pane
    if (rootPanes_.size() == 1) {
        activePane_ = rootPanes_.back().get();
    }
    
    return rootPanes_.back().get();
}

Pane* PaneManager::splitPane(Pane* pane, SplitDirection direction) {
    if (!pane || !pane->session) {
        return nullptr; // Cannot split an invalid or non-leaf pane
    }

    // Create a new child pane
    auto newChild = std::make_unique<Pane>();
    newChild->id = nextPaneId_++;
    newChild->session = std::make_unique<TerminalSession>(
        pane->session->getRows(), pane->session->getCols(), renderer_, &settings_->getCurrentColorScheme()
    );
    newChild->parent = pane;

    // Move the existing session into another child pane
    auto existingChild = std::make_unique<Pane>();
    existingChild->id = nextPaneId_++; // Give it a new ID, though it's conceptually the old pane
    existingChild->session = std::move(pane->session); // Move the session
    existingChild->parent = pane;

    // Clear the parent's session, as it's now a container pane
    pane->session.reset();
    pane->splitDirection = direction;

    pane->children.push_back(std::move(existingChild));
    pane->children.push_back(std::move(newChild));

    // Update dimensions and resize children
    // (Actual geometric recalculation will happen in render pass based on split direction)

    return pane->children.back().get(); // Return the newly created pane
}

void PaneManager::closePane(Pane* pane) {
    if (!pane) return;

    // If it's a root pane
    if (!pane->parent) {
        // Find and remove from rootPanes_
        for (auto it = rootPanes_.begin(); it != rootPanes_.end(); ++it) {
            if (it->get() == pane) {
                // If it was the active pane, try to find another active pane
                if (activePane_ == pane && !rootPanes_.empty()) {
                    if (it == rootPanes_.begin()) {
                        activePane_ = (rootPanes_.size() > 1) ? rootPanes_[1].get() : nullptr;
                    } else {
                        activePane_ = rootPanes_.front().get();
                    }
                }
                rootPanes_.erase(it);
                break;
            }
        }
        if (rootPanes_.empty()) {
            activePane_ = nullptr; // No panes left
        }
        return;
    }

    // If it's a child pane
    if (pane->parent && !pane->parent->children.empty()) {
        Pane* parent = pane->parent;
        // Find and remove the pane from its parent's children
        for (auto it = parent->children.begin(); it != parent->children.end(); ++it) {
            if (it->get() == pane) {
                parent->children.erase(it);
                break;
            }
        }

        // If only one child remains, absorb its session into the parent
        if (parent->children.size() == 1) {
            Pane* remainingChild = parent->children.front().get();
            parent->session = std::move(remainingChild->session);
            parent->children.clear(); // Parent is now a leaf again
            parent->splitDirection = SplitDirection::Horizontal; // Default or undefined
        }

        // Update active pane if the closed pane was active
        if (activePane_ == pane) {
            if (!parent->children.empty()) {
                activePane_ = parent->children.front().get();
            } else if (parent->session) {
                activePane_ = parent;
            } else {
                activePane_ = rootPanes_.empty() ? nullptr : rootPanes_.front().get();
            }
        }
    }
}


void PaneManager::setActivePane(Pane* pane) {
    if (pane) {
        activePane_ = pane;
    }
}

Pane* PaneManager::getPaneById(int id) {
    for (const auto& root : rootPanes_) {
        Pane* found = findPaneRecursive(root.get(), id);
        if (found) return found;
    }
    return nullptr;
}

Pane* PaneManager::findPaneRecursive(Pane* current, int id) {
    if (!current) return nullptr;
    if (current->id == id) return current;

    for (const auto& child : current->children) {
        Pane* found = findPaneRecursive(child.get(), id);
        if (found) return found;
    }
    return nullptr;
}


void PaneManager::update() {
    fd_set readfds;
    FD_ZERO(&readfds);
    int maxFd = -1;
    
    // Collect FDs from all active terminal sessions in all panes
    for (const auto& rootPane : rootPanes_) {
        // Recursive helper to get all session FDs
        std::function<void(Pane*)> collectFds = 
            [&](Pane* pane) {
            if (!pane) return;
            if (pane->session && pane->session->getMasterFd() >= 0) {
                int fd = pane->session->getMasterFd();
                FD_SET(fd, &readfds);
                if (fd > maxFd) maxFd = fd;
            }
            for (const auto& child : pane->children) {
                collectFds(child.get());
            }
        };
        collectFds(rootPane.get());
    }
    
    if (maxFd >= 0) {
        struct timeval timeout;
        timeout.tv_sec = 0;
        timeout.tv_usec = 0;
        
        if (select(maxFd + 1, &readfds, nullptr, nullptr, &timeout) > 0) {
            // Process output for all sessions that have data
            for (const auto& rootPane : rootPanes_) {
                std::function<void(Pane*)> processPaneOutput = 
                    [&](Pane* pane) {
                    if (!pane) return;
                    if (pane->session && pane->session->getMasterFd() >= 0) {
                        int fd = pane->session->getMasterFd();
                        if (FD_ISSET(fd, &readfds)) {
                            char buffer[4096];
                            int bytesRead = read(fd, buffer, sizeof(buffer));
                            if (bytesRead > 0) {
                                pane->session->processOutput(std::string(buffer, bytesRead));
                            }
                        }
                    }
                    for (const auto& child : pane->children) {
                        processPaneOutput(child.get());
                    }
                };
                processPaneOutput(rootPane.get());
            }
        }
    }
}

void PaneManager::render(float x, float y, float width, float height) {
    // Start recursive rendering from each root pane (representing a tab)
    // For now, assume only one root pane or render them tiled at the top level
    // (This simplified approach ignores tiling logic for multiple root panes for now)
    if (!rootPanes_.empty()) {
        renderPane(rootPanes_.front().get(), x, y, width, height);
    }
}

void PaneManager::renderPane(Pane* pane, float x, float y, float width, float height) {
    if (!pane) return;

    pane->x = x;
    pane->y = y;
    pane->width = width;
    pane->height = height;

    if (pane->session) {
        // Render the terminal session content using Application's method
        app_->drawTerminalContent(pane->session.get(), pane->x, pane->y, pane->width, pane->height);
    } else {
        // This is a container pane, render its children
        if (!pane->children.empty()) {
            if (pane->splitDirection == SplitDirection::Vertical) {
                float childWidth = width / pane->children.size();
                for (size_t i = 0; i < pane->children.size(); ++i) {
                    renderPane(pane->children[i].get(), x + i * childWidth, y, childWidth, height);
                }
            } else { // Horizontal
                float childHeight = height / pane->children.size();
                for (size_t i = 0; i < pane->children.size(); ++i) {
                    renderPane(pane->children[i].get(), x, y + i * childHeight, width, childHeight);
                }
            }
        }
    }

    // Optionally render borders around panes (e.g., green for active, gray for others)
    // if (pane == activePane_) {
    //     renderer_->renderBorder(pane->x, pane->y, pane->width, pane->height, 0.0f, 1.0f, 0.0f);
    // } else {
    //     renderer_->renderBorder(pane->x, pane->y, pane->width, pane->height, 0.5f, 0.5f, 0.5f);
    // }
}