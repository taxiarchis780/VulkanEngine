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
#include "DescriptorSet.h"
#include "Command.h"
#include "Resource.h"
#include "Framebuffer.h"
#include "Buffer.h"
#include "Texture.h"
#include "ResourceBuffer.h"
#include "SyncObject.h"
#include "Model.h"
#include "Camera.h"
#include "GraphicsPipeline.h"
#include "File.h"
#include "World.h"


#include <imgui.h>
#include <imgui_stdlib.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>
#include <ImGuizmo.h>
#include <tinylogger.h>
#include <json.hpp>

#include <Audio.h>
#include <PhysicsEngine.h>

#include <memory>
#include <iostream>
#include <filesystem>
#include <sys/stat.h>
#include <chrono>
#include <thread>
#include <mutex>
#include <stdexcept>
#include <vector>

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
    
    HWND getHWND();

    bool framebufferResized = false;
    bool swapChainConfigChanged = false;
    bool enableimGUI = true;

    
    VkInstance instance;

    std::vector<Model*> scene;
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
    std::string normal_path;
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
        
    double lastTimeWindowTitle = 0.0; // for window title
    double lastTime = 0.0; // for physics to be processed
    double fps = 0.0;
    int nbFrames = 0;
    int currentPipeline = 0;

    ImGuizmo::OPERATION mCurrentGizmoOperation;
    ImGuizmo::MODE mCurrentGizmoMode;

    
    size_t sceneSize;
    Model* mCurrentSelectedModel;
    
    ImGuiIO io;  
    Audio* audioMgr;
    PhysicsEngine* physicsEngine;

    GLFWwindow* window;
    GLFWmonitor* monitor;
    const GLFWvidmode* videoMode;
    HWND hwnd;
    
    VkPhysicalDevice physicalDevice;
    VkDevice device;
    VkQueue graphicsQueue;
    VkQueue presentQueue;
    VkSurfaceKHR surface;
    
    it_SwapChainHandle swapChainHandle;

    Camera* camera;
    
    VkFramebuffer shadowFramebuffer;
    
    VkRenderPass renderPass;
    VkRenderPass shadowRenderPass;

    std::vector<VkPipeline> graphicsPipelines;
    std::vector<VkPipelineLayout> pipelineLayouts;

    VkPipeline shadowPipeline;
    VkPipelineLayout shadowPipelineLayout;

    VkCommandPool commandPool;
    VkDescriptorPool descriptorPool;
    it_ImageResource depthImageRes;
    it_ImageResource colorImageRes;
    it_ImageResource shadowImageRes;
    it_lightBufferResource lightRes;
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
    
    void mainLoop();
    
    void drawFrame();
    
    void drawWindowTitle();
    void renderImGui();
    void loadShaders();
    void resetScene(bool toUpdate = false);
    void traceDir(std::string modelDirectory, std::string textureDirectory);
    void createImGuiDP();
    
    void updateImGui(VkCommandBuffer commandBuffer);
    void processState();
    

    //bool hasStencilComponent(VkFormat format);
    void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);
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