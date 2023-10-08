#ifndef __ENGINE_CLASS__
#define __ENGINE_CLASS__
#define VK_USE_PLATFORM_WIN32_KHR
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>
#include <vulkan/vulkan.h>
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <imgui.h>
#include <imgui_stdlib.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>
#include <ImGuizmo.h>
#include <ThreadManager.h>
#include <tinylogger.h>
#include <json.hpp>
#include <Model.h>
#include <Camera.h>
#include <iostream>
#include <filesystem>
#include <sys/stat.h>
#include <chrono>
#include <thread>
#include <mutex>
#include <stdexcept>
#include <vector>
#include <set>
#include <array>
#include <optional>
#include <cstring>
#include <cstdint>
#include <limits>
#include <algorithm>
#include <cstdlib>


using json = nlohmann::ordered_json;


struct UniformBufferObject
{
    glm::mat4 transform;
    glm::mat4 view;
    glm::mat4 proj;
    glm::vec3 cameraPos;
    glm::vec3 lightPos;
    glm::vec3 lightColor;
    Material material;
};


class Engine {

public:

    Engine(uint32_t width, uint32_t height, char* title, char* version);
    void run();
    bool framebufferResized = false;
    bool enableimGUI = true;

private:
    uint32_t WIDTH, HEIGHT = 0;
    std::vector<std::string> model_paths;
    std::vector<std::string> texture_paths;
    std::vector<std::string> scene_paths;
    std::string RendererName;
    bool addModel = false;
    bool editScene = false;
    bool firstScene = true;
    
    std::string texture_path;
    std::string model_path;
    std::string scene_path;
    std::string scene_name;
    int currentItemInListBox = 0;
    uint32_t currentFrame = 0;
    const char* TITLE;
    const char* VERSION;
    const int MAX_FRAMES_IN_FLIGHT = 2;
    bool enableValidationLayers = true;
    bool LockImGui = false;
    bool lightTranslateEnable = false;
    bool shouldDestroy = false;
    bool shouldResetScene = false;
    bool shouldUpdatePipeline = true;
    int pipelineIndex = 0;
    float FOV = 90.0f;
    float RotDegrees;
    double lastTime = 0.0;
    double lastTime1 = 0.0;
    uint32_t statsFaces = 0;
    int nbFrames = 0;

    std::vector<std::thread*> workers;
    std::mutex mutex;
    
    size_t sceneSize;
    Model* mCurrentSelectedModel;
    std::vector<Model*> scene;
    ImGuiIO io;
    GLFWwindow* window;
    GLFWmonitor* monitor;
    VkInstance instance;
    VkDebugUtilsMessengerEXT debugMessenger;
    VkPhysicalDevice physicalDevice;
    VkDevice device;
    VkQueue graphicsQueue;
    VkQueue presentQueue;
    VkSurfaceKHR surface;
    VkSwapchainKHR swapChain;
    VkDescriptorSetLayout descriptorSetLayout;

    int currentPipeline = 0;
    bool lockPipeline = true;
    VkRenderPass renderPass;
    //VkPipeline graphicsPipeline;
    std::vector<VkPipeline> graphicsPipelines;
    std::vector<VkPipelineLayout> pipelineLayouts;
    //VkPipelineLayout pipelineLayout;
    
    VkCommandPool commandPool;
    VkDescriptorPool descriptorPool;
    VkImage depthImage;
    VkDeviceMemory depthImageMemory;
    VkImageView depthImageView;
    VkImage colorImage;
    VkDeviceMemory colorImageMemory;
    VkImageView colorImageView;
    VkSampleCountFlagBits msaaSamples = VK_SAMPLE_COUNT_1_BIT;  
    VkDescriptorPool imGuiDP;
    std::vector<VkCommandBuffer> commandBuffers;
    std::vector<VkImage> swapChainImages;
    VkFormat swapChainImageFormat;
    VkExtent2D swapChainExtent;
    Camera* camera;
    std::vector<VkSemaphore> imageAvailableSemaphores;
    std::vector<VkSemaphore> renderFinishedSemaphores;
    std::vector<VkFence> inFlightFences;
    std::vector<VkImageView> swapChainImageViews;
    std::vector<VkFramebuffer> swapChainFramebuffers;
    std::vector<VkDescriptorSet> descriptorSets;
    ThreadMgr* thrdMgr;
    
    struct QueueFamilyIndices
    {
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
    const std::vector<const char*> deviceExtensions =
    {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
        //VK_NV_RAY_TRACING_EXTENSION_NAME
    };
    const std::vector<const char*> validationLayers = {
        "VK_LAYER_KHRONOS_validation"
    };

    
    void writeToFile(std::vector<Model*> scene);
    void loadFile(std::string fileName);
    json j;
    void enableDebugging() { enableValidationLayers = !enableValidationLayers; };
    void initWindow();
    void initVulkan();
    void initImGui();
    void recreateSwapChain();
    void cleanupSwapChain();
    void setupDebugMessenger();
    void mainLoop();
    void createInstance();
    void createSurface();
    void pickPhysicalDevice();
    void createLogicalDevice();
    void createSwapChain();
    void createImageViews();
    void createRenderPass();
    void createGraphicsPipeline(int index, std::string vertShaderPath, std::string fragShaderPath);
    void createGraphicsPipelineWrapper(std::string vertShaderPath, std::string fragShaderPath);
    void createFramebuffers();
    void createCommandPool();
    void createCommandBuffers();
    void createSyncObjects();
    void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory);
    void createVertexBuffer(Model* model);
    void createIndexBuffer(Model* model);
    void createUniformBuffers(Model* model);
    void updateUniformBuffers(uint32_t currentImage, Model* model);
    void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
    void createDescriptorSetLayout();
    void createDescriptorSets(Model* model);
    void createDescriptorPool(Model* model);
    void drawFrame();
    void loadModel(Model* model);
    void createTextureImage(Model* model);
    void createTextureImageView(Model* model);
    void createTextureSampler(Model* model);
    void createDepthResources();
    void transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t mipLevels);
    void createImage(uint32_t width, uint32_t height, uint32_t mipLevels, VkSampleCountFlagBits numSamples,VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory);
    void drawWindowTitle();
    void createColorResources();
    void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);
    void renderImGui();
    void loadScene();
    void resetScene();
    void addtoScene(Model* model);
    void traceDir(std::string modelDirectory, std::string textureDirectory);
    bool isDeviceSuitable(VkPhysicalDevice device);
    bool checkDeviceExtensionSupport(VkPhysicalDevice device);
    void createImGuiDP();
    void traceScenesDir(std::string sceneDirectory);
    void cleanUpModel(Model* model);
    int rateDeviceSuitability(VkPhysicalDevice device);
    void updateImGui(VkCommandBuffer commandBuffer);
    void generateMipmaps(VkImage image, VkFormat imageFormat, int32_t texWidth, int32_t texHeight, uint32_t mipLevels);
    VkImageView createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, uint32_t mipLevels);
    QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);
    SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device);
    VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
    VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
    VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilitiesd);
    VkCommandBuffer beginSingleTimeCommands();
    void endSingleTimeCommands(VkCommandBuffer commandBuffer);
    bool checkValidationLayerSupport();
    VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger);
    void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator);
    void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);
    std::vector<const char*> getRequiredExtensions();
    uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
    VkShaderModule createShaderModule(const std::vector<char>& code);
    VkFormat findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);
    VkFormat findDepthFormat();
    VkSampleCountFlagBits getMaxUsableSampleCount();
    //bool hasStencilComponent(VkFormat format);
    void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);
    void cleanup();



    
};

#endif