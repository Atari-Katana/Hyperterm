# Add the test executable
add_executable(hyperterm_tests
    settings_test.cpp
    ${CMAKE_SOURCE_DIR}/src/application.cpp
    ${CMAKE_SOURCE_DIR}/src/renderer/vulkan_renderer.cpp
    ${CMAKE_SOURCE_DIR}/src/renderer/font_renderer.cpp
    ${CMAKE_SOURCE_DIR}/src/renderer/image_loader.cpp
    ${CMAKE_SOURCE_DIR}/src/terminal/terminal_session.cpp
    ${CMAKE_SOURCE_DIR}/src/terminal/terminal_manager.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/menu_bar.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/window_tiler.cpp
    ${CMAKE_SOURCE_DIR}/src/settings/settings.cpp
    ${CMAKE_SOURCE_DIR}/src/settings/settings_ui.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/pane_manager.cpp
)

# Link against Google Test
target_link_libraries(hyperterm_tests PRIVATE
    GTest::gtest_main
    ${VULKAN_LIBRARIES}
    ${Vulkan_LIBRARIES}
    glfw
    ${FREETYPE_LIBRARIES}
    pthread
    dl
)

# Include directories
target_include_directories(hyperterm_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${VULKAN_INCLUDE_DIRS}
    ${FREETYPE_INCLUDE_DIRS}
    ${CMAKE_SOURCE_DIR}/src/renderer
)

# Discover and add tests to CTest
include(GoogleTest)
gtest_discover_tests(hyperterm_tests)
