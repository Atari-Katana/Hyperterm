#pragma once

#include "pane.hpp"
#include <memory>
#include <vector>
#include <functional> // For std::function

class VulkanRenderer;
class Settings; // Forward declaration
class Application; // Forward declaration


class PaneManager {
public:
    PaneManager(Application* app, VulkanRenderer* renderer, Settings* settings);
    ~PaneManager();

    // Session/Pane management
    Pane* createRootPane();
    Pane* splitPane(Pane* pane, SplitDirection direction);
    void closePane(Pane* pane);
    
    // Navigation
    void setActivePane(Pane* pane);
    Pane* getActivePane() const { return activePane_; }

    // Tree traversal/rendering
    void update(); // Update all terminal sessions
    void render(float x, float y, float width, float height); // Render all panes recursively

    // Get a specific pane by its ID (useful for external interaction)
    Pane* getPaneById(int id);

private:
    Application* app_;
    VulkanRenderer* renderer_;
    Settings* settings_; // To get color scheme, font, etc.

    std::vector<std::unique_ptr<Pane>> rootPanes_; // Each root pane represents a 'tab'
    Pane* activePane_;
    int nextPaneId_;

    // Helper functions for recursive operations on the pane tree
    void renderPane(Pane* pane, float x, float y, float width, float height);
    void updatePane(Pane* pane);

    // Helper to find a pane recursively
    Pane* findPaneRecursive(Pane* current, int id);
};