#pragma once

#include "renderer/vulkan_renderer.hpp"
#include "renderer/font_renderer.hpp"
#include "renderer/image_loader.hpp"
#include "terminal/terminal_manager.hpp"
#include "terminal/terminal_session.hpp"
#include "ui/pane_manager.hpp" // Replaced tab_manager.hpp
#include "ui/menu_bar.hpp"
#include "ui/window_tiler.hpp"
#include "settings/settings.hpp"
#include "settings/settings_ui.hpp"
#include <memory>
#include <string>
#include <vector>

struct GLFWwindow;

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
    
private:
    GLFWwindow* window_;
    std::unique_ptr<VulkanRenderer> renderer_;
    std::unique_ptr<FontRenderer> fontRenderer_;
    std::unique_ptr<TerminalManager> terminalManager_; // Will be removed, but temporarily here
    std::unique_ptr<PaneManager> paneManager_; // Replaced tabManager_
    std::unique_ptr<MenuBar> menuBar_;
    std::unique_ptr<WindowTiler> windowTiler_;
    std::unique_ptr<Settings> settings_;
    std::unique_ptr<SettingsUI> settingsUI_;
    
    bool isTiled_;
    std::vector<TileRect> tileRects_;
    int scrollOffset_;
    
    bool isSelecting_;
    SelectionCoord selectionStart_;
    SelectionCoord selectionEnd_;
    
    // Search functionality
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
public: // Made public for PaneManager to call
    void drawTerminalContent(TerminalSession* session, float x, float y, float width, float height);
    void renderSearchUI(float windowWidth, float windowHeight); // Declaration added

    std::string getSelectedText();
    void toggleSearch();
    void findAllMatches(); // Declaration added
    void findNextMatch();
    void findPreviousMatch();
    
    static void keyCallback(GLFWwindow* window, int key, [[maybe_unused]] int scancode, int action, int mods);
    static void charCallback(GLFWwindow* window, unsigned int codepoint);
    static void mouseButtonCallback(GLFWwindow* window, int button, int action, [[maybe_unused]] int mods);
    static void cursorPosCallback(GLFWwindow* window, double xpos, double ypos);
    static void scrollCallback(GLFWwindow* window, double xoffset, double yoffset);
    
    void onNewTab(); // Added back
    void onCloseTab(); // Added back
    void onQuit();
    void onSettings();
    void onTile(); // Added back
    bool isPathSafe(const std::string& path);

    static constexpr float MENU_BAR_HEIGHT = 30.0f;
};