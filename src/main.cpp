#include "application.hpp"
#include <iostream>
#include <exception>

int main() {
    try {
        Application app;
        
        if (!app.init()) {
            std::cerr << "Failed to initialize application" << std::endl;
            return 1;
        }
        
        app.run();
        app.cleanup();
        
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}

