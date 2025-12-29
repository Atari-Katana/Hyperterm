#pragma once

#include <memory>
#include <string>
#include <vector>

// Explicitly include full definitions for all types used directly by Application
#include "renderer/vulkan_renderer.hpp"
#include "renderer/font_renderer.hpp"
#include "renderer/image_loader.hpp" 
#include "terminal/terminal_session.hpp"
#include "ui/pane_manager.hpp"
#include "ui/menu_bar.hpp"
#include "ui/window_tiler.hpp"
#include "settings/settings.hpp"
#include "settings/settings_ui.hpp"

// Forward declaration for GLFWwindow, as it's an external type
struct GLFWwindow;

// Define SelectionCoord directly here, as it's a simple struct and only used within Application
struct SelectionCoord {
    int row;
    int col;
};

class Application {
public:
    Application();
    ~Application();
    
    bool init();
    void run();
    void cleanup();
    
public: // Made public for PaneManager to call
    void drawTerminalContent(TerminalSession* session, float x, float y, float width, float height);
    
private:
    GLFWwindow* window_;
    std::unique_ptr<Settings> settings_;
    std::unique_ptr<WindowTiler> windowTiler_;
    std::unique_ptr<SettingsUI> settingsUI_;
    std::unique_ptr<MenuBar> menuBar_;
    std::unique_ptr<PaneManager> paneManager_;
    std::unique_ptr<FontRenderer> fontRenderer_;
    std::unique_ptr<VulkanRenderer> renderer_; // MUST be last - destroyed last!
    
    bool isTiled_;
    std::vector<TileRect> tileRects_; // TileRect is defined in window_tiler.hpp
    int scrollOffset_;
    
    bool isSelecting_;
    SelectionCoord selectionStart_;
    SelectionCoord selectionEnd_;
    
    bool isSearching_;
    std::string searchQuery_;
    std::vector<SelectionCoord> searchResultCoords_;
    int currentSearchResultIndex_;
    
    void initWindow();
    void initVulkan();
    void initGraphics();
    void initSubsystems();
    void mainLoop();
    void drawFrame();
    void handleInput();
    void renderSearchUI(float windowWidth, float windowHeight);

    std::string getSelectedText();
    void toggleSearch();
    void findAllMatches();
    void findNextMatch();
    void findPreviousMatch();
    
    static void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
    static void charCallback(GLFWwindow* window, unsigned int codepoint);
    static void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
    static void cursorPosCallback(GLFWwindow* window, double xpos, double ypos);
    static void scrollCallback(GLFWwindow* window, double xoffset, double yoffset);
    
    void onNewTab();
    void onCloseTab();
    void onQuit();
    void onSettings();
    void onTile();
    bool isPathSafe(const std::string& path);

    static constexpr float MENU_BAR_HEIGHT = 30.0f;
};