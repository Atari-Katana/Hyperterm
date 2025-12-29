#include "application.hpp"
#include "ui/window_tiler.hpp"
#include "ui/pane_manager.hpp" // Changed
#include "renderer/stb_image.h"
#include <GLFW/glfw3.h>
#include <stdexcept>
#include <iostream>
#include <cstdlib>
#include <climits>
#include <algorithm> // Required for std::min, std::max, std::swap

namespace {
    std::string codepointToUtf8(unsigned int codepoint) {
        std::string out;
        if (codepoint <= 0x7f) {
            out.push_back(static_cast<char>(codepoint));
        } else if (codepoint <= 0x7ff) {
            out.push_back(static_cast<char>(0xc0 | (codepoint >> 6)));
            out.push_back(static_cast<char>(0x80 | (codepoint & 0x3f)));
        } else if (codepoint <= 0xffff) {
            out.push_back(static_cast<char>(0xe0 | (codepoint >> 12)));
            out.push_back(static_cast<char>(0x80 | ((codepoint >> 6) & 0x3f)));
            out.push_back(static_cast<char>(0x80 | (codepoint & 0x3f)));
        } else {
            out.push_back(static_cast<char>(0xf0 | (codepoint >> 18)));
            out.push_back(static_cast<char>(0x80 | ((codepoint >> 12) & 0x3f)));
            out.push_back(static_cast<char>(0x80 | ((codepoint >> 6) & 0x3f)));
            out.push_back(static_cast<char>(0x80 | (codepoint & 0x3f)));
        }
        return out;
    }
}

Application::Application() : window_(nullptr), isTiled_(false), scrollOffset_(0), 
                           isSelecting_(false), selectionStart_({0,0}), selectionEnd_({0,0}),
                           isSearching_(false), currentSearchResultIndex_(-1) {
}

Application::~Application() {
    cleanup();
}

bool Application::init() {
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return false;
    }
    
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
    
    try {
        initGraphics();
        initSubsystems();
    } catch (const std::exception& e) {
        std::cerr << "Initialization error: " << e.what() << std::endl;
        cleanup();
        return false;
    }
    
    // Create initial pane
    Pane* initialPane = paneManager_->createRootPane();
    if (initialPane) {
        paneManager_->setActivePane(initialPane);
    }
    
    return true;
}

void Application::initGraphics() {
    initWindow();
    initVulkan();
}

void Application::initSubsystems() {
    settings_ = std::make_unique<Settings>();
    const char* homeDir = getenv("HOME");
    std::string configPath;
    if (homeDir) {
        configPath = std::string(homeDir) + "/.hyperterm/config";
    } else {
        std::cerr << "Warning: HOME environment variable not set. Using current directory for config." << std::endl;
        configPath = "./.hyperterm/config";
    }
    settings_->load(configPath);
    
    paneManager_ = std::make_unique<PaneManager>(this, renderer_.get(), settings_.get()); // Changed to pass 'this'
    menuBar_ = std::make_unique<MenuBar>();
    windowTiler_ = std::make_unique<WindowTiler>();
    settingsUI_ = std::make_unique<SettingsUI>(settings_.get());
    
    // Setup settings UI with renderer (after Vulkan is initialized)
    settingsUI_->setRenderer(renderer_.get());
    settingsUI_->setFontRenderer(fontRenderer_.get());
    
    // Setup menu bar with renderer
    menuBar_->setRenderer(renderer_.get());
    menuBar_->setFontRenderer(fontRenderer_.get());
    
    // New menu bar actions (Adapted for PaneManager)
    menuBar_->onNewTab = [this]() { 
        Pane* newPane = paneManager_->createRootPane();
        paneManager_->setActivePane(newPane);
    };
    menuBar_->onCloseTab = [this]() { 
        if (paneManager_->getActivePane()) {
            paneManager_->closePane(paneManager_->getActivePane());
        }
    };
    menuBar_->onQuit = [this]() { onQuit(); };
    menuBar_->onSettings = [this]() { onSettings(); };
    menuBar_->onTile = [this]() { onTile(); }; // onTile still uses TabManager for now
}

void Application::initWindow() {
    window_ = glfwCreateWindow(1024, 768, "Hyperterm", nullptr, nullptr);
    if (!window_) {
        throw std::runtime_error("Failed to create GLFW window");
    }
    
    glfwSetWindowUserPointer(window_, this);
    glfwSetKeyCallback(window_, keyCallback);
    glfwSetCharCallback(window_, charCallback);
    glfwSetMouseButtonCallback(window_, mouseButtonCallback);
    glfwSetCursorPosCallback(window_, cursorPosCallback);
    glfwSetScrollCallback(window_, scrollCallback);
}

void Application::initVulkan() {
    renderer_ = std::make_unique<VulkanRenderer>(window_);
    renderer_->init();
    
    fontRenderer_ = std::make_unique<FontRenderer>(
        renderer_->getDevice(),
        renderer_->getPhysicalDevice(),
        renderer_->getGraphicsQueue(),
        renderer_->getCommandPool()
    );
    
    std::string fontPath = settings_->getFontPath();
    if (!fontPath.empty()) {
        fontRenderer_->loadFont(fontPath, settings_->getFontSize());
    }
}

void Application::run() {
    mainLoop();
}

void Application::mainLoop() {
    while (!glfwWindowShouldClose(window_)) {
        glfwPollEvents();
        handleInput();
        paneManager_->update(); // Changed
        drawFrame();
    }
    
    vkDeviceWaitIdle(renderer_->getDevice());
}

void Application::drawFrame() {
    renderer_->beginFrame();
    
    // Render menu bar
    float width = static_cast<float>(renderer_->getWidth());
    float height = static_cast<float>(renderer_->getHeight());
    menuBar_->render(width, height);
    
    // Render panes using PaneManager
    paneManager_->render(0.0f, MENU_BAR_HEIGHT, width, height - MENU_BAR_HEIGHT); // Changed
    
    // Render settings UI if visible
    if (settingsUI_->isVisible()) {
        settingsUI_->render(width, height);
    }

    // Render search UI if active
    if (isSearching_) {
        renderSearchUI(width, height);
    }
    
    renderer_->endFrame();
}

void Application::renderSearchUI(float windowWidth, float windowHeight) {
    if (!renderer_ || !fontRenderer_) return;

    float searchUIHeight = 50.0f;
    float searchUIY = windowHeight - searchUIHeight;
    float padding = 10.0f;

    // Background for search bar
    renderer_->renderQuad(0, searchUIY, windowWidth, searchUIHeight, VK_NULL_HANDLE, 0.15f, 0.15f, 0.15f, 0.9f);

    // Render search query
    fontRenderer_->renderString(padding, searchUIY + padding, "Search: " + searchQuery_, 1.0f, 1.0f, 1.0f);

    // Render search results count
    std::string resultText = "";
    if (!searchResultCoords_.empty()) {
        resultText = std::to_string(currentSearchResultIndex_ + 1) + "/" + std::to_string(searchResultCoords_.size());
    } else if (!searchQuery_.empty()) {
        resultText = "No results";
    }

    float resultTextWidth = fontRenderer_->getTextWidth(resultText);
    fontRenderer_->renderString(windowWidth - resultTextWidth - padding, searchUIY + padding, resultText, 1.0f, 1.0f, 1.0f);
}


void Application::handleInput() {
    // Input handling is done via callbacks
}

// drawTerminalContent is called by PaneManager to render a specific session
void Application::drawTerminalContent(TerminalSession* session, float x, float y, float width, float height) {
    if (!session || !fontRenderer_) return; 
    
    // Render background image if set
    const std::string& bgImage = session->getBackgroundImage();
    if (!bgImage.empty()) {
        if (!isPathSafe(bgImage)) {
            std::cerr << "Error: background image path is not safe: " << bgImage << std::endl;
        } else {
            VkImageView bgImageView = session->getBackgroundImage_();
            if (bgImageView != VK_NULL_HANDLE) {
                renderer_->renderQuad(x, y, width, height, bgImageView, 1.0f, 1.0f, 1.0f, 1.0f);
            }
        }
    }
    
    // --- Rendering with Scrollback and Selection ---
    uint32_t rows = session->getRows();
    uint32_t cols = session->getCols();
    float cellWidth = width / cols;
    float cellHeight = height / rows;
    
    const auto& scrollback = session->getScrollback();
    const auto& cells = session->getCells();
    
    int scrollbackSize = static_cast<int>(scrollback.size());
    
    // Clamp scroll offset
    if (scrollOffset_ > scrollbackSize) {
        scrollOffset_ = scrollbackSize;
    }
    
    // Calculate the starting line in the combined buffer
    int startLine = scrollbackSize - scrollOffset_;

    // Order selection points
    SelectionCoord orderedStart = selectionStart_;
    SelectionCoord orderedEnd = selectionEnd_;
    if (orderedStart.row > orderedEnd.row || (orderedStart.row == orderedEnd.row && orderedStart.col > orderedEnd.col)) {
        std::swap(orderedStart, orderedEnd);
    }
    
    for (uint32_t i = 0; i < rows; ++i) { // i is the screen row
        int lineIndex = startLine + i;
        const std::vector<Cell>* line = nullptr;
        
        if (lineIndex >= 0 && lineIndex < scrollbackSize) {
            line = &scrollback[lineIndex];
        } else if (lineIndex >= scrollbackSize && lineIndex < scrollbackSize + (int)rows) {
            line = &cells[lineIndex - scrollbackSize];
        } else {
            continue;
        }
        
        for (uint32_t j = 0; j < cols && j < line->size(); ++j) { // j is the screen col
            const auto& cell = (*line)[j];
            
            float cellX = x + j * cellWidth;
            float cellY = y + i * cellHeight;
            
            // Check if cell is selected
            bool isSelected = false;
            if (isSelecting_ || (selectionStart_.row != selectionEnd_.row || selectionStart_.col != selectionEnd_.col)) {
                int screenRow = static_cast<int>(i);
                int screenCol = static_cast<int>(j);

                if (screenRow > orderedStart.row && screenRow < orderedEnd.row) {
                    isSelected = true;
                } else if (screenRow == orderedStart.row && screenRow == orderedEnd.row) {
                    if (screenCol >= orderedStart.col && screenCol < orderedEnd.col) {
                        isSelected = true;
                    }
                } else if (screenRow == orderedStart.row) {
                    if (screenCol >= orderedStart.col) {
                        isSelected = true;
                    }
                } else if (screenRow == orderedEnd.row) {
                    if (screenCol < orderedEnd.col) {
                        isSelected = true;
                    }
                }
            }

            float r = ((cell.fgColor >> 16) & 0xFF) / 255.0f;
            float g = ((cell.fgColor >> 8) & 0xFF) / 255.0f;
            float b = (cell.fgColor & 0xFF) / 255.0f;

            bool isSearchMatch = false;
            if (isSearching_ && currentSearchResultIndex_ != -1 && !searchResultCoords_.empty()) {
                int matchAbsoluteRow = searchResultCoords_[currentSearchResultIndex_].row;
                int matchCol = searchResultCoords_[currentSearchResultIndex_].col;
                // Convert screen row to absolute row for comparison
                int currentAbsoluteRow = startLine + i;

                if (currentAbsoluteRow == matchAbsoluteRow && (int)j == matchCol) {
                    isSearchMatch = true;
                }
            }

            if (isSelected) {
                // Render highlight by swapping fg/bg
                renderer_->renderQuad(cellX, cellY, cellWidth, cellHeight, VK_NULL_HANDLE, r, g, b, 1.0f);

                float bg_r = ((cell.bgColor >> 16) & 0xFF) / 255.0f;
                float bg_g = ((cell.bgColor >> 8) & 0xFF) / 255.0f;
                float bg_b = (cell.bgColor & 0xFF) / 255.0f;
                if (cell.character != ' ' && cell.character != 0) {
                    fontRenderer_->renderCharacter(cellX, cellY, cell.character, bg_r, bg_g, bg_b);
                }
            } else if (isSearchMatch) {
                // Render search match highlight (e.g., yellow background)
                renderer_->renderQuad(cellX, cellY, cellWidth, cellHeight, VK_NULL_HANDLE, 1.0f, 1.0f, 0.0f, 0.5f); // Semi-transparent yellow
                if (cell.character != ' ' && cell.character != 0) {
                    fontRenderer_->renderCharacter(cellX, cellY, cell.character, r, g, b);
                }
            } else {
                // Render normal background (or nothing if transparent)
                // Assuming background is handled by the initial clear or background image

                // Render character
                if (cell.character != ' ' && cell.character != 0) {
                    fontRenderer_->renderCharacter(cellX, cellY, cell.character, r, g, b);
                }
            }
        }
    }
    
    // Only render the cursor if we are not scrolled up
    if (scrollOffset_ == 0) {
        uint32_t cursorRow = session->getCursorRow();
        uint32_t cursorCol = session->getCursorCol();
        if (cursorRow < rows && cursorCol < cols) {
            float cursorX = x + cursorCol * cellWidth;
            float cursorY = y + cursorRow * cellHeight + cellHeight - 2.0f;
            renderer_->renderQuad(cursorX, cursorY, cellWidth, 2.0f, VK_NULL_HANDLE, 1.0f, 1.0f, 1.0f, 1.0f);
        }
    }
}

void Application::onNewTab() {
    // This logic needs to be replaced by paneManager_->createRootPane()
    // For now, this is a placeholder.
    Pane* newPane = paneManager_->createRootPane();
    paneManager_->setActivePane(newPane);
}

void Application::onCloseTab() {
    // This logic needs to be replaced by paneManager_->closePane()
    if (paneManager_->getActivePane()) {
        paneManager_->closePane(paneManager_->getActivePane());
    }
}

void Application::onQuit() {
    glfwSetWindowShouldClose(window_, GLFW_TRUE);
}

void Application::onSettings() {
    settingsUI_->show();
}

void Application::onTile() {
    isTiled_ = !isTiled_;
    if (isTiled_) {
        // This will need to be adapted for paneManager
        float width = static_cast<float>(renderer_->getWidth());
        float height = static_cast<float>(renderer_->getHeight());
        //windowTiler_->tileWindows(paneManager_->getTabCount(), width, height, MENU_BAR_HEIGHT, tileRects_); // Needs adaptation
    }
}

void Application::cleanup() {
    if (renderer_) {
        renderer_->cleanup();
    }
    if (window_) {
        glfwDestroyWindow(window_);
    }
    glfwTerminate();
}

bool Application::isPathSafe(const std::string& path) {
    // Prevent absolute paths
    if (path.rfind('/', 0) == 0) {
        return false;
    }
    
    // Prevent directory traversal
    if (path.find("..") != std::string::npos) {
        return false;
    }
    
    return true;
}

void Application::keyCallback(GLFWwindow* window, int key, [[maybe_unused]] int scancode, int action, int mods) {
    auto* app = reinterpret_cast<Application*>(glfwGetWindowUserPointer(window));
    
    // Handle settings UI first
    if (app->settingsUI_->isVisible()) {
        if (app->settingsUI_->handleKey(key, action)) {
            return;
        }
    }
    
    if (action == GLFW_PRESS) {
        // Handle search keybinds
        if (mods == (GLFW_MOD_CONTROL | GLFW_MOD_SHIFT) && key == GLFW_KEY_F) { // Ctrl+Shift+F
            app->toggleSearch();
            return;
        }

        if (app->isSearching_) {
            if (key == GLFW_KEY_ESCAPE) {
                app->isSearching_ = false;
                app->searchQuery_.clear();
                app->searchResultCoords_.clear();
                app->currentSearchResultIndex_ = -1;
                return;
            } else if (key == GLFW_KEY_ENTER || key == GLFW_KEY_F3) {
                if (mods & GLFW_MOD_SHIFT) { // Shift+F3
                    app->findPreviousMatch();
                } else { // F3 or Enter
                    app->findNextMatch();
                }
                return;
            } else if (key == GLFW_KEY_BACKSPACE) {
                if (!app->searchQuery_.empty()) {
                    app->searchQuery_.pop_back();
                    app->findNextMatch(); // Re-search with new query
                }
                return;
            }
            // Other keys will be handled by charCallback for query input
            return;
        }
        
        // Handle Copy/Paste
        if (mods == (GLFW_MOD_CONTROL | GLFW_MOD_SHIFT)) {
            if (key == GLFW_KEY_C) { // Copy
                std::string selectedText = app->getSelectedText();
                if (!selectedText.empty()) {
                    glfwSetClipboardString(app->window_, selectedText.c_str());
                }
                // Clear selection
                app->isSelecting_ = false;
                app->selectionStart_ = {0,0};
                app->selectionEnd_ = {0,0};
                return;
            }
            if (key == GLFW_KEY_V) { // Paste
                const char* clipboardText = glfwGetClipboardString(app->window_);
                if (clipboardText) {
                    TerminalSession* activeSession = app->paneManager_->getActivePane()->session.get(); // Changed
                    if (activeSession) {
                        activeSession->writeInput(clipboardText);
                    }
                }
                return;
            }
        }

        app->scrollOffset_ = 0; // Reset scroll on key press
        if (app->menuBar_->handleKey(key, mods)) {
            return;
        }
        
        // Forward to active terminal
        TerminalSession* activeSession = app->paneManager_->getActivePane()->session.get(); // Changed
        if (activeSession) {
            // Convert key to string and send to terminal
            std::string code;
            switch (key) {
                case GLFW_KEY_UP: code = "\x1b[A"; break;
                case GLFW_KEY_DOWN: code = "\x1b[B"; break;
                case GLFW_KEY_RIGHT: code = "\x1b[C"; break;
                case GLFW_KEY_LEFT: code = "\x1b[D"; break;
                case GLFW_KEY_HOME: code = "\x1b[H"; break;
                case GLFW_KEY_END: code = "\x1b[F"; break;
                case GLFW_KEY_PAGE_UP: code = "\x1b[5~"; break;
                case GLFW_KEY_PAGE_DOWN: code = "\x1b[6~"; break;
                case GLFW_KEY_INSERT: code = "\x1b[2~"; break;
                case GLFW_KEY_DELETE: code = "\x1b[3~"; break;
                case GLFW_KEY_ENTER: code = "\r"; break;
                case GLFW_KEY_TAB: code = "\t"; break;
                case GLFW_KEY_BACKSPACE: code = "\x7f"; break;
                default: break;
            }
            if (!code.empty()) {
                activeSession->writeInput(code);
            }
        }
    }
}

void Application::charCallback(GLFWwindow* window, unsigned int codepoint) {
    auto* app = reinterpret_cast<Application*>(glfwGetWindowUserPointer(window));
    app->scrollOffset_ = 0; // Reset scroll on input
    
    if (app->isSearching_) {
        std::string utf8Char = codepointToUtf8(codepoint);
        app->searchQuery_ += utf8Char;
        app->findNextMatch(); // Re-search with new query
        return;
    }

    TerminalSession* activeSession = app->paneManager_->getActivePane()->session.get(); // Changed
    if (activeSession) {
        std::string input = codepointToUtf8(codepoint);
        activeSession->writeInput(input);
    }
}

void Application::mouseButtonCallback(GLFWwindow* window, int button, int action, [[maybe_unused]] int mods) {
    auto* app = reinterpret_cast<Application*>(glfwGetWindowUserPointer(window));
    double xpos, ypos;
    glfwGetCursorPos(window, &xpos, &ypos);
    float x = static_cast<float>(xpos);
    float y = static_cast<float>(ypos);

    if (button == GLFW_MOUSE_BUTTON_LEFT) {
        if (action == GLFW_PRESS) {
            // Handle settings UI first (it's on top)
            if (app->settingsUI_->isVisible()) {
                if (app->settingsUI_->handleClick(x, y)) {
                    return;
                }
            }
            // Then menu bar
            if (app->menuBar_->handleClick(x, y)) {
                return;
            }

            // Start selection in terminal area
            TerminalSession* activeSession = app->paneManager_->getActivePane()->session.get(); // Changed
            if (activeSession) {
                float termY = y - MENU_BAR_HEIGHT;
                int row = static_cast<int>(termY / (app->renderer_->getHeight() / activeSession->getRows()));
                int col = static_cast<int>(x / (app->renderer_->getWidth() / activeSession->getCols()));
                
                app->isSelecting_ = true;
                app->selectionStart_ = {row, col};
                app->selectionEnd_ = {row, col};
            }
        } else if (action == GLFW_RELEASE) {
            app->isSelecting_ = false;
        }
    }
}

void Application::cursorPosCallback(GLFWwindow* window, double xpos, double ypos) {
    auto* app = reinterpret_cast<Application*>(glfwGetWindowUserPointer(window));
    if (app->isSelecting_) {
        TerminalSession* activeSession = app->paneManager_->getActivePane()->session.get(); // Changed
        if (activeSession) {
            float termY = static_cast<float>(ypos) - MENU_BAR_HEIGHT;
            int row = static_cast<int>(termY / (app->renderer_->getHeight() / activeSession->getRows()));
            int col = static_cast<int>(xpos / (app->renderer_->getWidth() / activeSession->getCols()));
            
            app->selectionEnd_ = {row, col};
        }
    }
}

std::string Application::getSelectedText() {
    TerminalSession* activeSession = paneManager_->getActivePane()->session.get(); // Changed
    if (!activeSession) return "";

    std::string selectedText;
    
    SelectionCoord orderedStart = selectionStart_;
    SelectionCoord orderedEnd = selectionEnd_;
    if (orderedStart.row > orderedEnd.row || (orderedStart.row == orderedEnd.row && orderedStart.col > orderedEnd.col)) {
        std::swap(orderedStart, orderedEnd);
    }
    
    const auto& scrollback = activeSession->getScrollback();
    const auto& cells = activeSession->getCells();
    int scrollbackSize = static_cast<int>(scrollback.size());
    int rows = activeSession->getRows();
    int cols = activeSession->getCols();
    int startLine = scrollbackSize - scrollOffset_;

    for (int i = orderedStart.row; i <= orderedEnd.row; ++i) {
        int lineIndex = startLine + i;
        const std::vector<Cell>* line = nullptr;

        if (lineIndex >= 0 && lineIndex < scrollbackSize) {
            line = &scrollback[lineIndex];
        } else if (lineIndex >= scrollbackSize && lineIndex < scrollbackSize + rows) {
            line = &cells[lineIndex - scrollbackSize];
        } else {
            continue;
        }

        int startCol = (i == orderedStart.row) ? orderedStart.col : 0;
        int endCol = (i == orderedEnd.row) ? orderedEnd.col : cols;

        for (int j = startCol; j < endCol && j < (int)line->size(); ++j) {
            selectedText += codepointToUtf8((*line)[j].character);
        }

        if (i < orderedEnd.row) {
            selectedText += '\n';
        }
    }

    return selectedText;
}

void Application::scrollCallback(GLFWwindow* window, [[maybe_unused]] double xoffset, double yoffset) {
    auto* app = reinterpret_cast<Application*>(glfwGetWindowUserPointer(window));
    
    TerminalSession* activeSession = app->paneManager_->getActivePane()->session.get(); // Changed
    if (!activeSession) return; 
    
    // yoffset is typically -1 for scroll down, +1 for scroll up
    app->scrollOffset_ -= static_cast<int>(yoffset);
    
    // Clamp scrollOffset
    if (app->scrollOffset_ < 0) {
        app->scrollOffset_ = 0;
    }
    int maxScroll = static_cast<int>(activeSession->getScrollbackSize());
    if (app->scrollOffset_ > maxScroll) {
        app->scrollOffset_ = maxScroll;
    }
}

void Application::toggleSearch() {
    isSearching_ = !isSearching_;
    if (!isSearching_) {
        searchQuery_.clear();
        searchResultCoords_.clear();
        currentSearchResultIndex_ = -1;
    } else {
        // When opening search, immediately find matches for existing query if any
        if (!searchQuery_.empty()) {
            findAllMatches();
            findNextMatch(); // Jump to first result
        }
    }
}

void Application::findAllMatches() {
    searchResultCoords_.clear();
    currentSearchResultIndex_ = -1;
    if (searchQuery_.empty()) return;

    TerminalSession* activeSession = paneManager_->getActivePane()->session.get(); // Changed
    if (!activeSession) return;

    const auto& scrollback = activeSession->getScrollback();
    const auto& cells = activeSession->getCells();
    int rows_count = activeSession->getRows();
    int cols_count = activeSession->getCols();
    int scrollbackSize = static_cast<int>(scrollback.size());

    // Search scrollback
    for (int r = 0; r < scrollbackSize; ++r) {
        const auto& line = scrollback[r];
        std::string lineStr;
        for (const auto& cell : line) {
            lineStr += codepointToUtf8(cell.character); // Convert to UTF-8 for string search
        }
        
        size_t pos = lineStr.find(searchQuery_, 0);
        while (pos != std::string::npos) {
            // Need to convert byte-offset 'pos' back to column index
            int col = 0;
            size_t byteCount = 0;
            for(int i = 0; i < (int)line.size(); ++i) {
                std::string cellChar = codepointToUtf8(line[i].character);
                if(byteCount == pos) {
                    col = i;
                    break;
                }
                byteCount += cellChar.length();
                if(byteCount > pos) { // Match starts in middle of multibyte char
                    col = i;
                    break;
                }
            }
            searchResultCoords_.push_back({r, col}); // Store absolute line index
            pos = lineStr.find(searchQuery_, pos + 1);
        }
    }

    // Search live cells
    for (int r = 0; r < rows_count; ++r) {
        const auto& line = cells[r];
        std::string lineStr;
        for (const auto& cell : line) {
            lineStr += codepointToUtf8(cell.character);
        }

        size_t pos = lineStr.find(searchQuery_, 0);
        while (pos != std::string::npos) {
            int col = 0;
            size_t byteCount = 0;
            for(int i = 0; i < (int)line.size(); ++i) {
                std::string cellChar = codepointToUtf8(line[i].character);
                if(byteCount == pos) {
                    col = i;
                    break;
                }
                byteCount += cellChar.length();
                if(byteCount > pos) {
                    col = i;
                    break;
                }
            }
            searchResultCoords_.push_back({scrollbackSize + r, col}); // Store absolute line index
            pos = lineStr.find(searchQuery_, pos + 1);
        }
    }
}

void Application::findNextMatch() {
    if (searchResultCoords_.empty()) {
        findAllMatches();
        if (searchResultCoords_.empty()) return;
    }

    currentSearchResultIndex_ = (currentSearchResultIndex_ + 1) % searchResultCoords_.size();
    
    // Scroll to make the match visible
    TerminalSession* activeSession = paneManager_->getActivePane()->session.get(); // Changed
    if (!activeSession) return; 
    
    int matchAbsoluteRow = searchResultCoords_[currentSearchResultIndex_].row;
    int scrollbackSize = static_cast<int>(activeSession->getScrollbackSize());

    if (matchAbsoluteRow < scrollbackSize) { // Match is in scrollback
        scrollOffset_ = scrollbackSize - matchAbsoluteRow;
    } else { // Match is in live cells
        scrollOffset_ = 0;
    }
}

void Application::findPreviousMatch() {
    if (searchResultCoords_.empty()) {
        findAllMatches();
        if (searchResultCoords_.empty()) return;
    }

    currentSearchResultIndex_--;
    if (currentSearchResultIndex_ < 0) {
        currentSearchResultIndex_ = searchResultCoords_.size() - 1;
    }
    
    // Scroll to make the match visible
    TerminalSession* activeSession = paneManager_->getActivePane()->session.get(); // Changed
    if (!activeSession) return; 
    
    int matchAbsoluteRow = searchResultCoords_[currentSearchResultIndex_].row;
    int scrollbackSize = static_cast<int>(activeSession->getScrollbackSize());

    if (matchAbsoluteRow < scrollbackSize) { // Match is in scrollback
        scrollOffset_ = scrollbackSize - matchAbsoluteRow;
    } else { // Match is in live cells
        scrollOffset_ = 0;
    }
}
