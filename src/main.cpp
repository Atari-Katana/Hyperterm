#include <iostream>
#include <exception>

#include "application.hpp"

int main() {
    Application app;
    if (!app.init()) {
        std::cerr << "Failed to initialize application" << std::endl;
        return EXIT_FAILURE;
    }

    try {
        app.run();
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}