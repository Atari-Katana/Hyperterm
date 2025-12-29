#pragma once

#include <vulkan/vulkan.h>
#include <vector>
#include <string>
#include <memory>
#include <cstdint>
#include <optional>
#include <array>

struct Vertex {
    float pos[2];
    float texCoord[2];
    float color[4];
    
    static VkVertexInputBindingDescription getBindingDescription() {
        VkVertexInputBindingDescription bindingDescription{};
        bindingDescription.binding = 0;
        bindingDescription.stride = sizeof(Vertex);
        bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
        return bindingDescription;
    }
    
    static std::array<VkVertexInputAttributeDescription, 3> getAttributeDescriptions() {
        std::array<VkVertexInputAttributeDescription, 3> attributeDescriptions{};
        
        attributeDescriptions[0].binding = 0;
        attributeDescriptions[0].location = 0;
        attributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
        attributeDescriptions[0].offset = offsetof(Vertex, pos);
        
        attributeDescriptions[1].binding = 0;
        attributeDescriptions[1].location = 1;
        attributeDescriptions[1].format = VK_FORMAT_R32G32_SFLOAT;
        attributeDescriptions[1].offset = offsetof(Vertex, texCoord);
        
        attributeDescriptions[2].binding = 0;
        attributeDescriptions[2].location = 2;
        attributeDescriptions[2].format = VK_FORMAT_R32G32B32A32_SFLOAT;
        attributeDescriptions[2].offset = offsetof(Vertex, color);
        
        return attributeDescriptions;
    }
};

struct GLFWwindow;

class VulkanRenderer {
public:
    VulkanRenderer(GLFWwindow* window);
    ~VulkanRenderer();
    
    // Non-copyable
    VulkanRenderer(const VulkanRenderer&) = delete;
    VulkanRenderer& operator=(const VulkanRenderer&) = delete;
    
    void init();
    void cleanup();
    
    void beginFrame();
    void endFrame();
    
    void createTexture(uint32_t width, uint32_t height, const void* data, VkImage& image, VkDeviceMemory& memory, VkImageView& view);
    void destroyTexture(VkImage image, VkDeviceMemory memory, VkImageView view);
    
    void renderQuad(float x, float y, float width, float height, VkImageView texture = VK_NULL_HANDLE, float r = 1.0f, float g = 1.0f, float b = 1.0f, float a = 1.0f, float u0 = 0.0f, float v0 = 0.0f, float u1 = 1.0f, float v1 = 1.0f);
    void renderText(float x, float y, const std::string& text, float r = 1.0f, float g = 1.0f, float b = 1.0f);
    
    uint32_t getCurrentImageIndex() const;
    
    VkDevice getDevice() const { return device_; }
    VkPhysicalDevice getPhysicalDevice() const { return physicalDevice_; }
    VkCommandBuffer getCurrentCommandBuffer() const { return commandBuffers_[currentFrame_]; }
    VkCommandPool getCommandPool() const { return commandPool_; }
    VkQueue getGraphicsQueue() const { return graphicsQueue_; }
    
    uint32_t getWidth() const { return swapChainExtent_.width; }
    uint32_t getHeight() const { return swapChainExtent_.height; }
    
    void recreateSwapChain();
    
private:
    GLFWwindow* window_;
    
    VkInstance instance_ = VK_NULL_HANDLE;
    VkDebugUtilsMessengerEXT debugMessenger_ = VK_NULL_HANDLE;
    VkSurfaceKHR surface_ = VK_NULL_HANDLE;
    VkPhysicalDevice physicalDevice_ = VK_NULL_HANDLE;
    VkDevice device_ = VK_NULL_HANDLE;
    VkQueue graphicsQueue_ = VK_NULL_HANDLE;
    VkQueue presentQueue_ = VK_NULL_HANDLE;
    VkSwapchainKHR swapChain_ = VK_NULL_HANDLE;
    std::vector<VkImage> swapChainImages_;
    VkFormat swapChainImageFormat_;
    VkExtent2D swapChainExtent_;
    std::vector<VkImageView> swapChainImageViews_;
    VkRenderPass renderPass_ = VK_NULL_HANDLE;
    VkDescriptorSetLayout descriptorSetLayout_ = VK_NULL_HANDLE;
    VkPipelineLayout pipelineLayout_ = VK_NULL_HANDLE;
    VkPipeline graphicsPipeline_ = VK_NULL_HANDLE;
    VkSampler textureSampler_ = VK_NULL_HANDLE;
    VkDescriptorPool descriptorPool_ = VK_NULL_HANDLE;
    std::vector<VkFramebuffer> swapChainFramebuffers_;
    VkCommandPool commandPool_ = VK_NULL_HANDLE;
    
    // Vertex buffer for quad rendering
    VkBuffer quadVertexBuffer_ = VK_NULL_HANDLE;
    VkDeviceMemory quadVertexBufferMemory_ = VK_NULL_HANDLE;
    std::vector<Vertex> quadVertices_;
    
    // White texture for solid colored quads
    VkImage whiteTexture_ = VK_NULL_HANDLE;
    VkDeviceMemory whiteTextureMemory_ = VK_NULL_HANDLE;
    VkImageView whiteTextureView_ = VK_NULL_HANDLE;
    std::vector<VkCommandBuffer> commandBuffers_;
    std::vector<VkSemaphore> imageAvailableSemaphores_;
    std::vector<VkSemaphore> renderFinishedSemaphores_;
    std::vector<VkFence> inFlightFences_;
    size_t currentFrame_ = 0;
    
    bool framebufferResized_ = false;
    
    void createInstance();
    void setupDebugMessenger();
    void createSurface();
    void pickPhysicalDevice();
    void createLogicalDevice();
    void createSwapChain();
    void createImageViews();
    void createRenderPass();
    void createDescriptorSetLayout();
    void createGraphicsPipeline();
    void createFramebuffers();
    void createCommandPool();
    void createCommandBuffers();
    void createSyncObjects();
    void createTextureSampler();
    void createDescriptorPool();
    void createQuadVertexBuffer();
    void cleanupQuadVertexBuffer();
    void createWhiteTexture();
    void cleanupWhiteTexture();
    
    void cleanupSwapChain();
    
    bool checkValidationLayerSupport();
    std::vector<const char*> getRequiredExtensions();
    void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);
    
    struct QueueFamilyIndices {
        std::optional<uint32_t> graphicsFamily;
        std::optional<uint32_t> presentFamily;
        
        bool isComplete() {
            return graphicsFamily.has_value() && presentFamily.has_value();
        }
    };
    
    struct SwapChainSupportDetails {
        VkSurfaceCapabilitiesKHR capabilities;
        std::vector<VkSurfaceFormatKHR> formats;
        std::vector<VkPresentModeKHR> presentModes;
    };
    
    bool isDeviceSuitable(VkPhysicalDevice device);
    bool checkDeviceExtensionSupport(VkPhysicalDevice device);
    QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);
    SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device);
    VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
    VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
    VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);
    
    VkShaderModule createShaderModule(const std::vector<char>& code);
    std::vector<char> readFile(const std::string& filename);
    
    uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
public: // Made public for FontRenderer
    void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory);
    VkCommandBuffer beginSingleTimeCommands();
    void endSingleTimeCommands(VkCommandBuffer commandBuffer);
private:
    void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
    
    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT messageType,
        const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
        void* pUserData);
    
    static void framebufferResizeCallback(GLFWwindow* window, int width, int height);
};

