#pragma once

#include <memory>
#include <vector>



enum class SplitDirection {
    Horizontal,
    Vertical
};

struct Pane {
    int id; // Unique identifier for the pane
    std::unique_ptr<TerminalSession> session; // Restored


    
    Pane* parent = nullptr;
    SplitDirection splitDirection; // Only valid if this pane has children
    std::vector<std::unique_ptr<Pane>> children;
    
    // Geometric properties for rendering
    float x, y, width, height;
};