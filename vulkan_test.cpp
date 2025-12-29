#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <iostream>
#include <vector>
#include <cstring>
#include <thread>
#include <chrono>

// Simple Vulkan diagnostic test program
// Tests each initialization step and reports results

bool checkValidationLayerSupport() {
    uint32_t layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

    std::vector<VkLayerProperties> availableLayers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

    std::cout << "Available Vulkan layers (" << layerCount << "):" << std::endl;
    for (const auto& layer : availableLayers) {
        std::cout << "  - " << layer.layerName << std::endl;
    }

    return layerCount > 0;
}

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "Vulkan Diagnostic Test" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << std::endl;

    // Step 1: Initialize GLFW
    std::cout << "[1/7] Initializing GLFW..." << std::endl;
    if (!glfwInit()) {
        std::cerr << "  ✗ FAILED: Could not initialize GLFW" << std::endl;
        return 1;
    }
    std::cout << "  ✓ SUCCESS: GLFW initialized" << std::endl;
    std::cout << std::endl;

    // Step 2: Check Vulkan support
    std::cout << "[2/7] Checking Vulkan support..." << std::endl;
    if (!glfwVulkanSupported()) {
        std::cerr << "  ✗ FAILED: Vulkan not supported" << std::endl;
        glfwTerminate();
        return 1;
    }
    std::cout << "  ✓ SUCCESS: Vulkan is supported" << std::endl;
    std::cout << std::endl;

    // Step 3: Check validation layers
    std::cout << "[3/7] Checking validation layers..." << std::endl;
    bool hasValidationLayers = checkValidationLayerSupport();
    if (hasValidationLayers) {
        std::cout << "  ✓ Validation layers available" << std::endl;
    } else {
        std::cout << "  ⚠ No validation layers (this is OK)" << std::endl;
    }
    std::cout << std::endl;

    // Step 4: Get required extensions
    std::cout << "[4/7] Getting required extensions..." << std::endl;
    uint32_t glfwExtensionCount = 0;
    const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    if (glfwExtensions == nullptr) {
        std::cerr << "  ✗ FAILED: Could not get required extensions" << std::endl;
        glfwTerminate();
        return 1;
    }

    std::cout << "  Required extensions (" << glfwExtensionCount << "):" << std::endl;
    for (uint32_t i = 0; i < glfwExtensionCount; i++) {
        std::cout << "    - " << glfwExtensions[i] << std::endl;
    }
    std::cout << "  ✓ SUCCESS: Extensions retrieved" << std::endl;
    std::cout << std::endl;

    // Step 5: Create Vulkan instance
    std::cout << "[5/7] Creating Vulkan instance..." << std::endl;
    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "Vulkan Test";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "No Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_0;

    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;
    createInfo.enabledExtensionCount = glfwExtensionCount;
    createInfo.ppEnabledExtensionNames = glfwExtensions;
    createInfo.enabledLayerCount = 0;

    VkInstance instance;
    VkResult result = vkCreateInstance(&createInfo, nullptr, &instance);

    if (result != VK_SUCCESS) {
        std::cerr << "  ✗ FAILED: Could not create Vulkan instance (error code: " << result << ")" << std::endl;
        glfwTerminate();
        return 1;
    }
    std::cout << "  ✓ SUCCESS: Vulkan instance created" << std::endl;
    std::cout << std::endl;

    // Step 6: Enumerate physical devices
    std::cout << "[6/7] Enumerating physical devices..." << std::endl;
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

    if (deviceCount == 0) {
        std::cerr << "  ✗ FAILED: No Vulkan-capable GPU found" << std::endl;
        vkDestroyInstance(instance, nullptr);
        glfwTerminate();
        return 1;
    }

    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

    std::cout << "  Found " << deviceCount << " device(s):" << std::endl;
    for (uint32_t i = 0; i < deviceCount; i++) {
        VkPhysicalDeviceProperties deviceProperties;
        vkGetPhysicalDeviceProperties(devices[i], &deviceProperties);
        std::cout << "    [" << i << "] " << deviceProperties.deviceName << std::endl;
        std::cout << "        API Version: " << VK_VERSION_MAJOR(deviceProperties.apiVersion) << "."
                  << VK_VERSION_MINOR(deviceProperties.apiVersion) << "."
                  << VK_VERSION_PATCH(deviceProperties.apiVersion) << std::endl;
        std::cout << "        Driver Version: " << deviceProperties.driverVersion << std::endl;
        std::cout << "        Vendor ID: 0x" << std::hex << deviceProperties.vendorID << std::dec << std::endl;
        std::cout << "        Device Type: ";
        switch (deviceProperties.deviceType) {
            case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
                std::cout << "Integrated GPU" << std::endl;
                break;
            case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
                std::cout << "Discrete GPU" << std::endl;
                break;
            case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:
                std::cout << "Virtual GPU" << std::endl;
                break;
            case VK_PHYSICAL_DEVICE_TYPE_CPU:
                std::cout << "CPU" << std::endl;
                break;
            default:
                std::cout << "Other" << std::endl;
        }
    }
    std::cout << "  ✓ SUCCESS: Physical devices enumerated" << std::endl;
    std::cout << std::endl;

    // Step 7: Create test window
    std::cout << "[7/7] Creating test window..." << std::endl;
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    GLFWwindow* window = glfwCreateWindow(800, 600, "Vulkan Test Window", nullptr, nullptr);
    if (!window) {
        std::cerr << "  ✗ FAILED: Could not create window" << std::endl;
        vkDestroyInstance(instance, nullptr);
        glfwTerminate();
        return 1;
    }
    std::cout << "  ✓ SUCCESS: Window created" << std::endl;
    std::cout << std::endl;

    // Step 8: Create surface
    std::cout << "[8/8] Creating window surface..." << std::endl;
    VkSurfaceKHR surface;
    result = glfwCreateWindowSurface(instance, window, nullptr, &surface);

    if (result != VK_SUCCESS) {
        std::cerr << "  ✗ FAILED: Could not create window surface (error code: " << result << ")" << std::endl;
        glfwDestroyWindow(window);
        vkDestroyInstance(instance, nullptr);
        glfwTerminate();
        return 1;
    }
    std::cout << "  ✓ SUCCESS: Window surface created" << std::endl;
    std::cout << std::endl;

    // Success!
    std::cout << "========================================" << std::endl;
    std::cout << "✓ ALL TESTS PASSED!" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << std::endl;
    std::cout << "Vulkan is working correctly on your system." << std::endl;
    std::cout << "The issue with Hyperterm is likely in the application code," << std::endl;
    std::cout << "not with your Vulkan installation." << std::endl;
    std::cout << std::endl;
    std::cout << "Keeping window open for 3 seconds..." << std::endl;

    // Keep window open briefly
    for (int i = 3; i > 0; i--) {
        std::cout << i << "..." << std::flush;
        glfwPollEvents();
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    std::cout << std::endl;

    // Cleanup
    vkDestroySurfaceKHR(instance, surface, nullptr);
    glfwDestroyWindow(window);
    vkDestroyInstance(instance, nullptr);
    glfwTerminate();

    return 0;
}
