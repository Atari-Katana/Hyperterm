cmake_minimum_required(VERSION 3.15)
project(Hyperterm VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find dependencies
find_package(Vulkan REQUIRED)
find_package(glfw3 REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(FREETYPE REQUIRED freetype2)

# Source files
set(SOURCES
    src/main.cpp
    src/application.cpp
    src/renderer/vulkan_renderer.cpp
    src/renderer/font_renderer.cpp
    src/renderer/image_loader.cpp
    src/terminal/terminal_session.cpp
    src/ui/menu_bar.cpp
    src/ui/window_tiler.cpp
    src/settings/settings.cpp
    src/settings/settings_ui.cpp
    src/ui/pane_manager.cpp
)

# Header files
set(HEADERS
    src/application.hpp
    src/renderer/vulkan_renderer.hpp
    src/renderer/font_renderer.hpp
    src/renderer/image_loader.hpp
    src/terminal/terminal_session.hpp
    src/ui/menu_bar.hpp
    src/ui/window_tiler.hpp
    src/settings/settings.hpp
    src/settings/settings_ui.hpp
    src/ui/pane.hpp
    src/ui/pane_manager.hpp
)

# Shader files
file(GLOB SHADER_FILES shaders/*.vert shaders/*.frag)

# Create executable
add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS} ${SHADER_FILES})

# Include directories
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${VULKAN_INCLUDE_DIRS}
    ${FREETYPE_INCLUDE_DIRS}
)


# Link libraries
target_link_libraries(${PROJECT_NAME}
    ${VULKAN_LIBRARIES}
    ${Vulkan_LIBRARIES}
    glfw
    ${FREETYPE_LIBRARIES}
    pthread
    dl
)

# Compiler-specific options
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra)
endif()

# Copy shaders to build directory
file(COPY ${SHADER_FILES} DESTINATION ${CMAKE_BINARY_DIR}/shaders)

# --- Testing ---
enable_testing()

include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
)
FetchContent_MakeAvailable(googletest)

add_subdirectory(tests)
