#ifndef __ENGINE_CLASS__
#define __ENGINE_CLASS__
#define VK_USE_PLATFORM_WIN32_KHR
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>
#include <vulkan/vulkan.h>

#include "Instance.h"
#include "Surface.h"
#include "PhysicalDevice.h"
#include "LogicalDevice.h"
#include "Image.h"
#include "Renderpass.h"
#include "DescriptorSetLayout.h"
#include "Command.h"
#include "Resource.h"
#include "Framebuffer.h"

#include <imgui.h>
#include <imgui_stdlib.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>
#include <ImGuizmo.h>
#include <tinylogger.h>
#include <json.hpp>
#include <Model.h>
#include <Camera.h>
#include <Audio.h>
#include <PhysicsEngine.h>
#include <util.h>
#include <memory>
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
#define ENGINE_LOAD_SCENE 1
#define ENGINE_RESET_SCENE 0

using json = nlohmann::ordered_json;

enum SCENE_STATE
{
    STATE_NOP               = 0x00,
    STATE_DESTROY_OBJECT    = 0x01,
    STATE_RESET_SCENE       = 0x02,
    STATE_UPDATE_SCENE      = 0x04, // DO NOT USE Used to differentiate STATE_RESET_SCENE and STATE_RESET_AND_UPDATE 
    STATE_UPDATE_PIPELINE   = 0x08,
    
    STATE_RESET_AND_UPDATE_SCENE = STATE_RESET_SCENE | STATE_UPDATE_SCENE
};



class Engine {

public:

    Engine(uint32_t width, uint32_t height, char* title, char* version);
    void run();
    void setCustomCameraFunction(std::function<void(GLFWwindow*,Camera*)> customCameraFunction);
    void setCustomKeyCallbackFunction(std::function<void(GLFWwindow*, int, int, int, int)> custom_key_callback);
    void setCustomScrollCallbackFunction(std::function<void(GLFWwindow*, double, double, Camera* camera)> custom_scroll_callback);
    void setCustomMousePositionCallbackFunction(std::function<void(GLFWwindow*, double, double)> custom_mouse_position_callback);
    void setCustomMouseButtonCallbackFunction(std::function<void(GLFWwindow*, int, int, int)> custom_mouse_button_callback);
    void setCustomMainUpdate(std::function<void(void)> custom_main_update);
    

    bool framebufferResized = false;
    bool swapChainConfigChanged = false;
    bool enableimGUI = true;

    
    VkInstance instance;
private:
    uint32_t WIDTH, HEIGHT = 0;
    std::vector<std::string> model_paths;
    std::vector<std::string> texture_paths;
    std::vector<std::string> scene_paths;
    std::string RendererName;

    json j;

    bool addModel = false;
    bool editScene = false;
    bool firstScene = true;
    
    std::string texture_path;
    std::string model_path;
    std::string scene_path;
    std::string scene_name;
    std::string vert_path;
    std::string frag_path;
    int currentItemInListBox = 0;
    uint32_t currentFrame = 0;
    const char* TITLE;
    const char* VERSION;
    const int MAX_FRAMES_IN_FLIGHT = 2;
#ifdef _NO_VALIDATION
    bool enableValidationLayers = false;
#else
    bool enableValidationLayers = true;
#endif

    bool lightTranslateEnable = false;


    SCENE_STATE state;
    bool enableAudio = true;

    float volume = 0.5f;

    int pipelineIndex = 0;
    
    bool VSync = false;
    bool PresentModeChange = false;
        
    double lastTime = 0.0; // for window title
    double lastTime1 = 0.0; // for physics to be processed
    double fps = 0.0;
    uint32_t statsFaces = 0;
    int nbFrames = 0;
    int currentPipeline = 0;

    ImGuizmo::OPERATION mCurrentGizmoOperation;
    ImGuizmo::MODE mCurrentGizmoMode;

    
    size_t sceneSize;
    Model* mCurrentSelectedModel;
    std::vector<Model*> scene;
    ImGuiIO io;  
    Audio* audioMgr;
    PhysicsEngine* physicsEngine;

    GLFWwindow* window;
    GLFWmonitor* monitor;
    const GLFWvidmode* videoMode;
    
    VkDebugUtilsMessengerEXT debugMessenger;
    VkPhysicalDevice physicalDevice;
    VkDevice device;
    VkQueue graphicsQueue;
    VkQueue presentQueue;
    VkSurfaceKHR surface;
    
    it_SwapChainHandle swapChainHandle;

    Camera* camera;
    
    
    VkRenderPass renderPass;
    std::vector<VkPipeline> graphicsPipelines;
    std::vector<VkPipelineLayout> pipelineLayouts;
    VkPipeline postProcessingGraphicsPipeline;
    VkPipelineLayout postProcessingGraphicsPipelineLayout;    
    VkDescriptorPool postProcessingDescriptionPool;
    VkDescriptorSetLayout postProcessingDescriptorSetLayout;
    VkDescriptorSet postProcessingDescriptorSet;
    VkCommandPool commandPool;
    VkDescriptorPool descriptorPool;
    it_ImageResource depthImageRes;
    it_ImageResource colorImageRes;
    VkSampleCountFlagBits msaaSamples = VK_SAMPLE_COUNT_1_BIT;  
    VkDescriptorPool imGuiDP;
    std::vector<VkCommandBuffer> commandBuffers;
    
    
    std::vector<VkSemaphore> imageAvailableSemaphores;
    std::vector<VkSemaphore> renderFinishedSemaphores;
    std::vector<VkFence> inFlightFences;
    
    VkDescriptorSetLayout descriptorSetLayout;
    
    std::function<void(GLFWwindow*,Camera*)> customCameraFunction;
    std::function<void(GLFWwindow*,int,int,int,int)> custom_key_callback;
    std::function<void(GLFWwindow*, double, double, Camera* camera)> custom_scroll_callback;
    std::function<void(GLFWwindow*, double, double)> custom_mouse_position_callback;
    std::function<void(GLFWwindow*, int, int, int)> custom_mouse_button_callback;
    std::function<void(void)> custom_main_update;

    
    const std::vector<const char*> validationLayers = {
        "VK_LAYER_KHRONOS_validation"
    };

    const std::vector<const char*> presentModes = {
        "VK_PRESENT_MODE_IMMEDIATE_KHR",
        "VK_PRESENT_MODE_MAILBOX_KHR",
        "VK_PRESENT_MODE_FIFO_KHR",
        "VK_PRESENT_MODE_FIFO_RELAXED_KHR"
    };

    std::vector<std::string> shader_paths;
    std::vector<std::vector<int>> shader_indices;
    
    
    
    void enableDebugging() { enableValidationLayers = !enableValidationLayers; };
    void initWindow();
    void initVulkan();
    void initImGui();
    
    void setupDebugMessenger();
    void mainLoop();

    
    
    void createGraphicsPipeline(int index, std::string vertShaderPath, std::string fragShaderPath);
    void createGraphicsPipelineWrapper(std::string vertShaderPath, std::string fragShaderPath);
    void createPostProcessingGraphicsPipeline(std::string vertShaderPath, std::string fragShaderPath);
    void createSyncObjects();
    void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory);
    void createVertexBuffer(Model* model);
    void createIndexBuffer(Model* model);
    void createUniformBuffers(Model* model);
    void updateUniformBuffers(uint32_t currentImage, Model* model);
    void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
    void createPostProcessingDescriptorSets();
    void createDescriptorSetLayoutForModel(Model* model);
    
    void createDescriptorSets(Model* model);
    void createDescriptorPool(Model* model);
    void drawFrame();
    void loadModel(Model* model);
    void createTextureImage(Model* model);
    void createTextureImageView(Model* model);
    void createTextureSampler(Model* model);
    
    void transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t mipLevels);
    void drawWindowTitle();
    void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);
    void renderImGui();
    void loadScene();
    void loadShaders();
    void writeToFile(std::vector<Model*> scene);
    void loadFile(std::string fileName);
    void deleteFile(std::string fileName);
    void resetScene(bool toUpdate = false);
    void addtoScene(Model* model);
    void traceDir(std::string modelDirectory, std::string textureDirectory);
    void createImGuiDP();
    void findFiles(std::string sceneDirectory, std::string fileExtension);
    void cleanUpModel(Model* model);
    void updateImGui(VkCommandBuffer commandBuffer);
    void generateMipmaps(VkImage image, VkFormat imageFormat, int32_t texWidth, int32_t texHeight, uint32_t mipLevels);
    void createNormal(Model* cModel);
    void processState();
    
    bool checkValidationLayerSupport();
    VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger);
    void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator);
    void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);
    std::vector<const char*> getRequiredExtensions();
    VkShaderModule createShaderModule(const std::vector<char>& code);
    VkSampleCountFlagBits getMaxUsableSampleCount();

    

    //bool hasStencilComponent(VkFormat format);
    void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);
    void Draw(Model* cModel, VkCommandBuffer commandBuffer, VkPipelineLayout graphicsPipelineLayout, int currentFrame);
    void cleanup();

    static void mouse_callback(GLFWwindow* window, int key, int action, int mods);
    static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
    static void custom_key_callback_wrapper(GLFWwindow* window, int key, int scancode, int action, int mods);
    static void custom_scroll_callback_wrapper(GLFWwindow* window, double xoffset, double yoffset);
    static void custom_mouse_position_callback_wrapper(GLFWwindow* window, double xpos, double ypos);
    static void custom_mouse_button_callback_wrapper(GLFWwindow* window, int button, int action, int mod);
    
    static void fmod_update(GLFWwindow* window);

};

#endif