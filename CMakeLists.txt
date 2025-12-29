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

# --- Shader Compilation ---
# Find shader compiler (glslc preferred, fallback to glslangValidator)
find_program(GLSLC glslc HINTS
    $ENV{VULKAN_SDK}/bin
    $ENV{VULKAN_SDK}/Bin
    ${Vulkan_GLSLC_EXECUTABLE})

find_program(GLSLANG glslangValidator HINTS
    $ENV{VULKAN_SDK}/bin
    $ENV{VULKAN_SDK}/Bin
    ${Vulkan_GLSLANG_VALIDATOR_EXECUTABLE})

if(GLSLC)
    set(SHADER_COMPILER ${GLSLC})
    set(SHADER_COMPILE_FLAGS "")
    message(STATUS "Found shader compiler: glslc at ${GLSLC}")
elseif(GLSLANG)
    set(SHADER_COMPILER ${GLSLANG})
    set(SHADER_COMPILE_FLAGS "-V")
    message(STATUS "Found shader compiler: glslangValidator at ${GLSLANG}")
else()
    message(WARNING "No shader compiler found (glslc or glslangValidator). Shaders will not be compiled.")
    message(WARNING "Install with: sudo apt install glslang-tools")
    set(SHADER_COMPILER "")
endif()

# Create shaders output directory
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/shaders)

# Compile each shader if compiler is available
if(SHADER_COMPILER)
    set(COMPILED_SHADERS "")

    foreach(SHADER ${SHADER_FILES})
        get_filename_component(SHADER_NAME ${SHADER} NAME)
        get_filename_component(SHADER_EXT ${SHADER} EXT)

        # Determine output filename based on shader type
        if(SHADER_EXT STREQUAL ".vert")
            string(REPLACE ".vert" "_vert.spv" SPIRV_NAME ${SHADER_NAME})
        elseif(SHADER_EXT STREQUAL ".frag")
            string(REPLACE ".frag" "_frag.spv" SPIRV_NAME ${SHADER_NAME})
        else()
            set(SPIRV_NAME "${SHADER_NAME}.spv")
        endif()

        set(SPIRV_OUTPUT ${CMAKE_BINARY_DIR}/shaders/${SPIRV_NAME})

        # Add custom command to compile shader
        add_custom_command(
            OUTPUT ${SPIRV_OUTPUT}
            COMMAND ${SHADER_COMPILER} ${SHADER_COMPILE_FLAGS} ${SHADER} -o ${SPIRV_OUTPUT}
            DEPENDS ${SHADER}
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
            COMMENT "Compiling shader: ${SHADER_NAME} -> ${SPIRV_NAME}"
            VERBATIM
        )

        list(APPEND COMPILED_SHADERS ${SPIRV_OUTPUT})
    endforeach()

    # Create a custom target for all compiled shaders
    add_custom_target(compile_shaders ALL DEPENDS ${COMPILED_SHADERS})

    # Make the main executable depend on compiled shaders
    add_dependencies(${PROJECT_NAME} compile_shaders)

    message(STATUS "Shader compilation configured for ${CMAKE_BUILD_TYPE} build")
else()
    # Fallback: just copy source shaders (won't work at runtime)
    file(COPY ${SHADER_FILES} DESTINATION ${CMAKE_BINARY_DIR}/shaders)
    message(WARNING "Shaders copied but not compiled - application may not work without manual shader compilation")
endif()

# --- Testing ---
enable_testing()

include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
)
FetchContent_MakeAvailable(googletest)

add_subdirectory(tests)
