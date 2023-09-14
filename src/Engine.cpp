#include "Engine.h"
#include <map>
#include <sstream>
#include "util.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>
#include <unordered_map>






static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messaegSeverit, VkDebugUtilsMessageTypeFlagsEXT messsageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData)
{
    std::cerr << "ERROR in validation layer: " << pCallbackData->pMessage << std::endl;

    return VK_FALSE;
}

static void framebufferResizeCallback(GLFWwindow* window, int width, int height)
{
    auto app = reinterpret_cast<Engine*>(glfwGetWindowUserPointer(window));
    app->framebufferResized = true;
}

void Engine::populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo)
{
    createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    createInfo.pfnUserCallback = debugCallback;
}

void Engine::setupDebugMessenger()
{
    if (!enableValidationLayers) return;

    VkDebugUtilsMessengerCreateInfoEXT createInfo;
    populateDebugMessengerCreateInfo(createInfo);
    if(CreateDebugUtilsMessengerEXT(instance, &createInfo, nullptr, &debugMessenger) != VK_SUCCESS)
    {
        throw std::runtime_error("ERROR: Failed to set up debug messenger!");
    }
}

void Engine::DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator)
{
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
    if (func != nullptr) {
        func(instance, debugMessenger, pAllocator);
    }
}

void Engine::run()
{
    
    initWindow();
    initVulkan();
    if (enableimGUI)
    {
        initImGui();
        traceDir("res/models/", "res/textures/");
    }
    mainLoop();
    cleanup();
}

Engine::Engine(uint32_t width, uint32_t height, char* title, char* version)
{
    WIDTH = width;
    HEIGHT = height;
    TITLE = title;
    VERSION = version;
}

void Engine::initWindow()
{
    glfwInit();
    
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

    window = glfwCreateWindow(WIDTH, HEIGHT, TITLE, nullptr, nullptr);
    monitor = glfwGetPrimaryMonitor();
    
    glfwSetWindowUserPointer(window, this);
    glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);
}

void Engine::initVulkan()
{
    createInstance();
    setupDebugMessenger();
    createSurface();
    pickPhysicalDevice();
    createLogicalDevice();
    createSwapChain();
    createImageViews();
    createRenderPass();
    createDescriptorSetLayout();
    createGraphicsPipeline();
    createCommandPool();
    createColorResources();
    createDepthResources();
    createFramebuffers();
    if(firstScene)
        scene_path = "main.json";
    camera = new Camera(swapChainExtent.width, swapChainExtent.height);
    loadScene();
    loadFile(scene_path);

    
    
    for (size_t i = 0; i < sceneSize; ++i)
    {
        createTextureImage(scene[i]);
        createTextureImageView(scene[i]);
        createTextureSampler(scene[i]);
        createVertexBuffer(scene[i]);
        createIndexBuffer(scene[i]);
        createUniformBuffers(scene[i]);
        createDescriptorPool(scene[i]);
        createDescriptorSets(scene[i]);
    }
    createImGuiDP();
    
    mCurrentSelectedModel = scene[0];

    createCommandBuffers();
    createSyncObjects();
}

void Engine::initImGui()
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange;   // Disable the fucking mouse
    
    io.MouseDrawCursor = false;
    ImGui_ImplGlfw_InitForVulkan(window, true);
    
    ImGui_ImplVulkan_InitInfo info{};
    info.Instance = instance;
    info.PhysicalDevice = physicalDevice;
    info.Device = device;
    info.Queue = graphicsQueue;
    info.DescriptorPool = imGuiDP;
    info.MinImageCount = MAX_FRAMES_IN_FLIGHT;
    info.ImageCount = MAX_FRAMES_IN_FLIGHT;
    info.MSAASamples = msaaSamples;
    ImGui_ImplVulkan_Init(&info, renderPass);
    VkCommandBuffer commandBuffer = beginSingleTimeCommands();
    ImGui_ImplVulkan_CreateFontsTexture(commandBuffer);
    endSingleTimeCommands(commandBuffer);
    vkDeviceWaitIdle(device);

    ImGui_ImplVulkan_DestroyFontUploadObjects();
}

void Engine::recreateSwapChain()
{
    int width = 0, height = 0;
    glfwGetFramebufferSize(window, &width, &height);
    while (width == 0 || height == 0)
    {
        glfwGetFramebufferSize(window, &width, &height);
        glfwWaitEvents();
    }

    vkDeviceWaitIdle(device);
    

    cleanupSwapChain();
    createSwapChain();
    camera->width = swapChainExtent.width;
    camera->height = swapChainExtent.height;
    createImageViews();
    createColorResources();
    createDepthResources();
    createFramebuffers();
}

void Engine::cleanupSwapChain()
{
    vkDestroyImageView(device, colorImageView, nullptr);
    vkDestroyImage(device, colorImage, nullptr);
    vkFreeMemory(device, colorImageMemory, nullptr);
    vkDestroyImageView(device, depthImageView, nullptr);
    vkDestroyImage(device, depthImage, nullptr);
    vkFreeMemory(device, depthImageMemory, nullptr);
    for (size_t i = 0; i < swapChainFramebuffers.size(); i++)
        vkDestroyFramebuffer(device, swapChainFramebuffers[i], nullptr);
    for (size_t i = 0; i < swapChainImageViews.size(); i++)
        vkDestroyImageView(device, swapChainImageViews[i], nullptr);
    vkDestroySwapchainKHR(device, swapChain, nullptr);
}

/// <summary>
/// You need dt in order to update everything at a specific time in order for physics (or anything that moves) to be not bound by the FPS
/// that the application in running at e.g. on a computer that the engine runs at 2K FPS the camera is going to move faster than a computer 
/// that is running at 60 FPS
/// </summary>
void Engine::mainLoop()
{
    while(!glfwWindowShouldClose(window))
    {
        glfwPollEvents();
        drawWindowTitle();
        double currentTime = glfwGetTime(); // shitty implementation of dt
        double deltaTime = currentTime - lastTime1;
        
        if (deltaTime >= 0.016) // update every 16 ms (updates 60 times per second)
        {
            camera->UpdateInputs(window);
            lastTime1 = currentTime;
        }
        
        drawFrame();
        
        
    }
    vkDeviceWaitIdle(device);
}

void Engine::createInstance()
{
    if(enableValidationLayers && !checkValidationLayerSupport()) {
        
        throw std::runtime_error("ERROR: validation layers requested, but not avaiable!");
    }
    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "Engine Application";
    appInfo.applicationVersion = VK_MAKE_VERSION(0, 0, 1);
    appInfo.pEngineName = "No Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(0, 0, 1);
    appInfo.apiVersion = VK_API_VERSION_1_3;

    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;
    uint32_t glfwExtensionCount = 0;
    const char** glfwExtensions;
    glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    auto extensions = getRequiredExtensions();
    createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    createInfo.ppEnabledExtensionNames = extensions.data();

    
    if(enableValidationLayers)
    {
        createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
        createInfo.ppEnabledLayerNames = validationLayers.data(); 
    } else {createInfo.enabledLayerCount = 0;}

    if(vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS)
    {
        throw std::runtime_error("ERROR: Failed to create instance!");
    }
}

void Engine::createSurface()
{
    if(glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS)
    {
        throw std::runtime_error("ERROR: failed to create window surface!");
    }
}


void Engine::pickPhysicalDevice()
{
    
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
    std::vector<VkPhysicalDevice> devices(deviceCount);
    VkResult res = vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());
    VkPhysicalDeviceProperties deviceProperties;
    
    if (deviceCount == 0) {
        throw std::runtime_error("ERROR: failed to find GPUs with Vulkan support!");
    }
    


    std::multimap<int, VkPhysicalDevice> candidates;
    
    for (const auto& device : devices)
    {
        int score = 0;
        vkGetPhysicalDeviceProperties(device, &deviceProperties);
        if(isDeviceSuitable(device))
        {
            score = rateDeviceSuitability(device); 
            candidates.insert(std::make_pair(score, device));
        }
        //printf("%s with score: %d\n", &deviceProperties.deviceName, score);
        std::stringstream ss;
        ss << deviceProperties.deviceName << " with score: " << score;

        tlog::info(ss.str());
    }
    
    
    
    if (candidates.rbegin()->first > 0) {
        physicalDevice = candidates.rbegin()->second;
        msaaSamples = getMaxUsableSampleCount();
    } else {
        throw std::runtime_error("ERROR: failed to find a suitable GPU!");
    }

    //This is done to find out the score
    
    vkGetPhysicalDeviceProperties(physicalDevice, &deviceProperties);
    
    //printf("Card: %s selected with score: %d\n", &deviceProperties.deviceName, candidates.rbegin()->first);
    std::stringstream ss;
    ss << "Card: " << deviceProperties.deviceName << " selected with score: " << candidates.rbegin()->first;
    tlog::none();
    tlog::info(ss.str());
}
bool Engine::isDeviceSuitable(VkPhysicalDevice device)
{
    QueueFamilyIndices indices = findQueueFamilies(device);
    bool extensionsSupported = checkDeviceExtensionSupport(device);
    bool swapChainAdequate = false;
    VkPhysicalDeviceFeatures supportedFeatures;
    vkGetPhysicalDeviceFeatures(device, &supportedFeatures);
    if(extensionsSupported)
    {
        SwapChainSupportDetails SwapChainSupport = querySwapChainSupport(device);
        swapChainAdequate = !SwapChainSupport.formats.empty() && !SwapChainSupport.presentModes.empty();
    }
    
    return indices.isComplete() && extensionsSupported && swapChainAdequate && supportedFeatures.samplerAnisotropy;
}
bool Engine::checkDeviceExtensionSupport(VkPhysicalDevice device)
{
    uint32_t extensionCount;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);
    std::vector<VkExtensionProperties> availableExtensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());
    std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());
    for(const auto& extension : availableExtensions)
    {
        requiredExtensions.erase(extension.extensionName);
    }
    return requiredExtensions.empty();
}
int Engine::rateDeviceSuitability(VkPhysicalDevice device)
{
    VkPhysicalDeviceProperties deviceProperties;
    VkPhysicalDeviceFeatures deviceFeatures;
    VkPhysicalDeviceMemoryProperties deviceMemoryProperties;
    
    vkGetPhysicalDeviceProperties(device, &deviceProperties);
    vkGetPhysicalDeviceFeatures(device, &deviceFeatures);
    vkGetPhysicalDeviceMemoryProperties(device, &deviceMemoryProperties);

    int score = 0;
    // Discrete GPUs have a significant performance advantage
    if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
        score += 1000;
    } else if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU)
    {
        score += 100;
    }
    // Maximum possible size of textures affects graphics quality
    score += deviceProperties.limits.maxImageDimension2D;
    // Application can't function without geometry shaders
    if (!deviceFeatures.geometryShader) {
        return 0;
    }
    // https://stackoverflow.com/questions/44339931/query-amount-of-vram-or-gpu-clock-speed
    auto heapsPointer = deviceMemoryProperties.memoryHeaps;
    auto heaps = std::vector<VkMemoryHeap>(heapsPointer, heapsPointer + deviceMemoryProperties.memoryHeapCount);
    //std::cout << deviceProperties.deviceName << std::endl;
    
    for (const auto& heap : heaps)
    {
        if (heap.flags & VkMemoryHeapFlagBits::VK_MEMORY_HEAP_DEVICE_LOCAL_BIT)
        {
            score += heap.size/100000; 
        }
    }

    return score;

}
Engine::QueueFamilyIndices Engine::findQueueFamilies(VkPhysicalDevice device)
{
    QueueFamilyIndices indices;
    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

    int i = 0;
    for(const auto& queueFamily : queueFamilies)
    {
        VkBool32 presentSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);
        if(queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {indices.graphicsFamily = i;}
        if(presentSupport) {indices.presentFamily = i;}
        if(indices.isComplete()) {break;}
        i++;
    }

    return indices;
}
Engine::SwapChainSupportDetails Engine::querySwapChainSupport(VkPhysicalDevice device)
{
    SwapChainSupportDetails details;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);

    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);

    if (formatCount != 0) {
        details.formats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.formats.data());
    }

    uint32_t presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);

    if (presentModeCount != 0) {
        details.presentModes.resize(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, details.presentModes.data());
    }

    return details;
}

VkSurfaceFormatKHR Engine::chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats)
{
    for(const auto& availableFormat : availableFormats)
    {
        if(availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
        {
            return availableFormat;
        }
    }
    return availableFormats[0];
}

VkPresentModeKHR Engine::chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes)
{
    for(const auto& availablePresentMode : availablePresentModes)
    {
        if (availablePresentMode == VK_PRESENT_MODE_IMMEDIATE_KHR) { return availablePresentMode; }
        //
    }   

    return VK_PRESENT_MODE_FIFO_KHR; //Shitty Vsync
}

VkExtent2D Engine::chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities)
{
    if(capabilities.currentExtent.width != (std::numeric_limits<uint32_t>::max)()) {return capabilities.currentExtent;}
    else {
        int width, height;
        glfwGetFramebufferSize(window, &width, &height);
        
        VkExtent2D actualExtent = {
            static_cast<uint32_t>(width),
            static_cast<uint32_t>(height)
        };

        actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
        actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

        return actualExtent;
    }
}

void Engine::createLogicalDevice()
{
    QueueFamilyIndices indices = findQueueFamilies(physicalDevice);
    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    std::set<uint32_t> uniqueQueueFamilies = {indices.graphicsFamily.value(), indices.presentFamily.value()};

    float queuePriority = 1.0f;
    for(uint32_t queueFamily : uniqueQueueFamilies)
    {
        VkDeviceQueueCreateInfo queueCreateInfo{};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = queueFamily;
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &queuePriority;
        queueCreateInfos.push_back(queueCreateInfo);
    }

    VkPhysicalDeviceFeatures deviceFeatures{};
    deviceFeatures.samplerAnisotropy = VK_TRUE;
    
    VkDeviceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
    createInfo.pQueueCreateInfos = queueCreateInfos.data();
    createInfo.pEnabledFeatures = &deviceFeatures;
    createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
    createInfo.ppEnabledExtensionNames = deviceExtensions.data();
    createInfo.enabledLayerCount = 0;
    if(enableValidationLayers)
    {
        createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
        createInfo.ppEnabledLayerNames = validationLayers.data();
    } else  {
        createInfo.enabledLayerCount = 0;
    }

    if(vkCreateDevice(physicalDevice, &createInfo, nullptr, &device)!= VK_SUCCESS)
    {
        throw std::runtime_error("ERROR: failed to create logical device!");
    }
    vkGetDeviceQueue(device, indices.graphicsFamily.value(), 0, &graphicsQueue);
    vkGetDeviceQueue(device, indices.presentFamily.value(), 0, &presentQueue);
}

void Engine::createSwapChain()
{
    SwapChainSupportDetails swapChainSupport = querySwapChainSupport(physicalDevice);
    VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
    VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
    VkExtent2D extent = chooseSwapExtent(swapChainSupport.capabilities);
    
    uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
    if(swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount){imageCount = swapChainSupport.capabilities.maxImageCount;}

    VkSwapchainCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = surface;
    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageExtent = extent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    QueueFamilyIndices indices = findQueueFamilies(physicalDevice);
    uint32_t QueueFamilyIndices[] = {indices.graphicsFamily.value(), indices.presentFamily.value()};

    if(indices.graphicsFamily != indices.presentFamily){
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = QueueFamilyIndices;
    } else {
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        createInfo.queueFamilyIndexCount = 0;
        createInfo.pQueueFamilyIndices = nullptr;
    }

    createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode = presentMode;
    createInfo.clipped = VK_TRUE;
    createInfo.oldSwapchain = VK_NULL_HANDLE;

    if(vkCreateSwapchainKHR(device, &createInfo, nullptr, &swapChain) != VK_SUCCESS)
    {
        throw std::runtime_error("ERROR: failed to create swap chain!");
    }

    vkGetSwapchainImagesKHR(device, swapChain, &imageCount, nullptr);
    swapChainImages.resize(imageCount);
    vkGetSwapchainImagesKHR(device, swapChain, &imageCount, swapChainImages.data());
    swapChainImageFormat = surfaceFormat.format;
    swapChainExtent = extent;
}

bool Engine::checkValidationLayerSupport()
{
    uint32_t layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
    std::vector<VkLayerProperties> availableLayers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());
    for(const char* layerName : validationLayers)
    {
        bool layerFound = false;

        for(const auto& layerProperties : availableLayers)
        {
            if(strcmp(layerName, layerProperties.layerName) == 0)
            {
                layerFound = true;
                break;
            }
        }
        if(!layerFound)
            return false;
    }
    return true;
}

std::vector<const char*> Engine::getRequiredExtensions()
{
    uint32_t glfwExtensionCount = 0;
    const char** glfwExtensions;
    glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
    std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);
    if(enableValidationLayers) {extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);}
    return extensions;
}

VkResult Engine::CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger)
{
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
    if(func != nullptr) {return func(instance, pCreateInfo, pAllocator, pDebugMessenger);}
    else { return VK_ERROR_EXTENSION_NOT_PRESENT;}
}

void Engine::createImageViews()
{
    swapChainImageViews.resize(swapChainImages.size());
    for (uint32_t i = 0; i < swapChainImages.size(); i++) {
        swapChainImageViews[i] = createImageView(swapChainImages[i], swapChainImageFormat, VK_IMAGE_ASPECT_COLOR_BIT, 1);
    }
}

VkShaderModule Engine::createShaderModule(const std::vector<char>& code)
{
    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = code.size();
    createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());
    VkShaderModule shaderModule;
    if(vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS)
    {
        throw std::runtime_error("ERROR: failed to create shader module!");
    }
    return shaderModule;
}

void Engine::createRenderPass()
{
    VkAttachmentDescription colorAttachment{};
    colorAttachment.format = swapChainImageFormat;
    colorAttachment.samples = msaaSamples;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentDescription depthAttachment{};
    depthAttachment.format = findDepthFormat();
    depthAttachment.samples = msaaSamples;
    depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentDescription colorAttachmentResolve{};
    colorAttachmentResolve.format = swapChainImageFormat;
    colorAttachmentResolve.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachmentResolve.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachmentResolve.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachmentResolve.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachmentResolve.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachmentResolve.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachmentResolve.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference colorAttachmentRef{};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentReference depthAttachmentRef{};
    depthAttachmentRef.attachment = 1;
    depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentReference colorAttachmentResolveRef{};
    colorAttachmentResolveRef.attachment = 2;
    colorAttachmentResolveRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;
    subpass.pDepthStencilAttachment = &depthAttachmentRef;
    subpass.pResolveAttachments = &colorAttachmentResolveRef;

    VkSubpassDependency dependency{};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

    std::array<VkAttachmentDescription, 3> attachments = { colorAttachment, depthAttachment, colorAttachmentResolve };
    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    renderPassInfo.pAttachments = attachments.data();
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;

    if (vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS) {
        throw std::runtime_error("ERROR: failed to create render pass!");
    }
}

void Engine::createGraphicsPipeline(){
    auto vertShaderCode = readFile("shaders/vert.spv");
    auto fragShaderCode = readFile("shaders/frag.spv");

    VkShaderModule vertShaderModule = createShaderModule(vertShaderCode);
    VkShaderModule fragShaderModule = createShaderModule(fragShaderCode);

    VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
    vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertShaderStageInfo.module = vertShaderModule;
    vertShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
    fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragShaderStageInfo.module = fragShaderModule;
    fragShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

    auto bindingDescription = Vertex::getBindingDescription();
    auto attributeDescriptions = Vertex::getAttributeDescriptions();

    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
    vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
    vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.scissorCount = 1;

    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = VK_CULL_MODE_NONE;//VK_CULL_MODE_BACK_BIT;
    rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_FALSE;

    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = msaaSamples;

    VkPipelineDepthStencilStateCreateInfo depthStencil{};
    depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencil.depthTestEnable = VK_TRUE;
    depthStencil.depthWriteEnable = VK_TRUE;
    depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
    depthStencil.depthBoundsTestEnable = VK_FALSE;
    depthStencil.stencilTestEnable = VK_FALSE;

    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_FALSE;

    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.logicOp = VK_LOGIC_OP_COPY;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;
    colorBlending.blendConstants[0] = 0.0f;
    colorBlending.blendConstants[1] = 0.0f;
    colorBlending.blendConstants[2] = 0.0f;
    colorBlending.blendConstants[3] = 0.0f;

    std::vector<VkDynamicState> dynamicStates = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };
    VkPipelineDynamicStateCreateInfo dynamicState{};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
    dynamicState.pDynamicStates = dynamicStates.data();

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout;

    if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
        throw std::runtime_error("ERROR: failed to create pipeline layout!");
    }

    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pDepthStencilState = &depthStencil;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDynamicState = &dynamicState;
    pipelineInfo.layout = pipelineLayout;
    pipelineInfo.renderPass = renderPass;
    pipelineInfo.subpass = 0;
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

    if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline) != VK_SUCCESS) {
        throw std::runtime_error("ERROR: failed to create graphics pipeline!");
    }

    vkDestroyShaderModule(device, fragShaderModule, nullptr);
    vkDestroyShaderModule(device, vertShaderModule, nullptr);
}

void Engine::createFramebuffers()
{
    swapChainFramebuffers.resize(swapChainImageViews.size());

    for (size_t i = 0; i < swapChainImageViews.size(); i++) {
        std::array<VkImageView, 3> attachments = {
            colorImageView,
            depthImageView,
            swapChainImageViews[i]
        };

        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = renderPass;
        framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
        framebufferInfo.pAttachments = attachments.data();
        framebufferInfo.width = swapChainExtent.width;
        framebufferInfo.height = swapChainExtent.height;
        framebufferInfo.layers = 1;

        if (vkCreateFramebuffer(device, &framebufferInfo, nullptr, &swapChainFramebuffers[i]) != VK_SUCCESS) {
            throw std::runtime_error("ERROR: failed to create framebuffer!");
        }
    }
}

void Engine::createCommandPool()
{
    QueueFamilyIndices queueFamilyIndices = findQueueFamilies(physicalDevice);

    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();

    if (vkCreateCommandPool(device, &poolInfo, nullptr, &commandPool) != VK_SUCCESS)
        throw std::runtime_error("ERROR: failed to create commandPool");
}

void Engine::createCommandBuffers()
{
    commandBuffers.resize(MAX_FRAMES_IN_FLIGHT);
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = commandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = (uint32_t) commandBuffers.size();

    if (vkAllocateCommandBuffers(device, &allocInfo, commandBuffers.data()) != VK_SUCCESS)
    {
        throw std::runtime_error("ERROR: failed to allocate command buffers!");
    }
}

void Engine::recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex)
{
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
        throw std::runtime_error("failed to begin recording command buffer!");
    }

    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = renderPass;
    renderPassInfo.framebuffer = swapChainFramebuffers[imageIndex];
    renderPassInfo.renderArea.offset = { 0, 0 };
    renderPassInfo.renderArea.extent = swapChainExtent;

    std::array<VkClearValue, 2> clearValues{};
    clearValues[0].color = { {0.00f, 0.00f, 0.001f, 1.0f} };
    clearValues[1].depthStencil = { 1.0f, 0 };

    renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
    renderPassInfo.pClearValues = clearValues.data();

    vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);

    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float)swapChainExtent.width;
    viewport.height = (float)swapChainExtent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

    VkRect2D scissor{};
    scissor.offset = { 0, 0 };
    scissor.extent = swapChainExtent;
    vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

    VkDeviceSize offsets[] = { 0 };

    for (size_t i = 0; i < scene.size(); ++i)
    {
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, &scene[i]->vertexBuffer, offsets);

        vkCmdBindIndexBuffer(commandBuffer, scene[i]->indexBuffer, 0, VK_INDEX_TYPE_UINT32);

        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &scene[i]->descriptorSets[currentFrame], 0, nullptr);

        vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(scene[i]->indices.size()), 1, 0, 0, 0);
    }

    if (glfwGetKey(window, GLFW_KEY_K) == GLFW_PRESS && !LockImGui)
    {
        enableimGUI = !enableimGUI;
        if (enableimGUI)
        {
            io.ConfigFlags &= ~ImGuiConfigFlags_NoMouseCursorChange;
        }
        else {
            io.ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange;
        }
        LockImGui = true;
    }
    if (glfwGetKey(window, GLFW_KEY_K) == GLFW_RELEASE)
    {
        LockImGui = false;
    }


    if (enableimGUI)
    {
        updateImGui(commandBuffer);
    }


    vkCmdEndRenderPass(commandBuffer);

    if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
        throw std::runtime_error("ERROR: failed to record command buffer!");
    }

    

}

void Engine::updateImGui(VkCommandBuffer commandBuffer)
{
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    renderImGui();
    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffer);
}

void Engine::renderImGui()
{
    static ImGuizmo::OPERATION mCurrentGizmoOperation(ImGuizmo::TRANSLATE);
    static ImGuizmo::MODE mCurrentGizmoMode(ImGuizmo::LOCAL);
    ImGui::NewFrame();

    
    ImGui::Begin("Components", 0, ImGuiWindowFlags_NoMove);
    ImGui::Text("CURRENT SELECTED MODEL");

    uint8_t count = 0;
    for (size_t i = 0; i < scene.size(); ++i)
    {
        count++;
        if (ImGui::RadioButton(std::to_string(i).c_str(), mCurrentSelectedModel == scene[i]))
            mCurrentSelectedModel = scene[i];
        if (count % 3 == 0)
        {
            count = 0;
            ImGui::Spacing();
            continue; 
        }
        
        ImGui::SameLine();
    }
    if (ImGui::Checkbox("LightTranslate", &lightTranslateEnable) && lightTranslateEnable)
    {
        mCurrentGizmoOperation = ImGuizmo::TRANSLATE;
    }
    
    ImGui::InputText("UUID", &mCurrentSelectedModel->UUID);
    if (ImGui::Button("Generate UUID"))
    {
        GenerateUUID(mCurrentSelectedModel, 8, true);
    }
    ImGui::Checkbox("EditScene", &editScene);
    if (editScene)
    {
        ImGui::InputText("SceneName", &scene_name);
        if (ImGui::Button("Refresh"))
        {
            traceScenesDir("res/data/");
        }
        if (ImGui::BeginListBox("Scenes", ImVec2(200, 100))) {
            static size_t item_current_idx = 0;
            for (size_t i = 0; i < scene_paths.size(); ++i)
            {
                const bool isSelected = (item_current_idx == i);
                if (ImGui::Selectable(scene_paths[i].c_str(), isSelected))
                {
                    item_current_idx = i;
                    scene_path = scene_paths[i];
                }
                if (isSelected)
                    ImGui::SetItemDefaultFocus();
            }
            ImGui::EndListBox();
        }
        if (ImGui::Button("LoadScene") && false) // using && to disable it // it is currently junky and unstable
        {
            vkDeviceWaitIdle(device);
            cleanup();
            firstScene = false;
            run();
            
        }
        if (ImGui::Button("SaveToSelected"))
        {
            writeToFile(scene);
        } ImGui::SameLine();
        if (ImGui::Button("SaveToInput"))
        {
            scene_path = scene_name;
            writeToFile(scene);
        }
    }
    
    if (ImGui::Button("Update Graphics Pipeline"))
    {
        shouldUpdatePipeline = true;
    }
    
    ImGui::SliderFloat("FOV", &FOV, 10.0f, 120.0f, NULL);
    ImGui::SliderFloat3("ModelPos", glm::value_ptr(mCurrentSelectedModel->translationVec), -5.0f, 5.0f, NULL);
    ImGui::SliderFloat3("ModelRot", glm::value_ptr(mCurrentSelectedModel->rotationVec), 0.0f, 6.28f, NULL);
    ImGui::SliderFloat3("ModelScale", glm::value_ptr(mCurrentSelectedModel->scaleVec), 0.001f, 10.0f, NULL);
    ImGui::SliderFloat3("LightPos", glm::value_ptr(camera->lightPos), -5.0f, 5.0f, NULL);
    
    

    ImGui::Text("CURRENT GIZMO OPERATION:");
    if (ImGui::RadioButton("Translate", mCurrentGizmoOperation == ImGuizmo::TRANSLATE))
        mCurrentGizmoOperation = ImGuizmo::TRANSLATE;
    ImGui::SameLine();
    if (ImGui::RadioButton("Rotation", mCurrentGizmoOperation == ImGuizmo::ROTATE))
        mCurrentGizmoOperation = ImGuizmo::ROTATE;
    ImGui::SameLine();
    if (ImGui::RadioButton("Scale", mCurrentGizmoOperation == ImGuizmo::SCALE))
        mCurrentGizmoOperation = ImGuizmo::SCALE;
    ImGui::SameLine();
    if (ImGui::RadioButton("Universal", mCurrentGizmoOperation == ImGuizmo::UNIVERSAL))
        mCurrentGizmoOperation = ImGuizmo::UNIVERSAL;
    


    //ImGuiColorEditFlags_PickerHueWheel
    ImGui::ColorPicker3("lightColor", glm::value_ptr(camera->lightColor), ImGuiColorEditFlags_DisplayRGB | ImGuiColorEditFlags_NoAlpha);
    ImGui::Text("Selected Model Material");
    ImGui::SliderFloat3("Ambient", glm::value_ptr(mCurrentSelectedModel->material.ambient), 0.0f, 10.0f, NULL);
    ImGui::SliderFloat3("Diffuse", glm::value_ptr(mCurrentSelectedModel->material.diffuse), 0.0f, 10.0f, NULL);
    ImGui::SliderFloat3("Specular", glm::value_ptr(mCurrentSelectedModel->material.specular), 0.0f, 10.0f, NULL);
    ImGui::SliderFloat("Shininess", &mCurrentSelectedModel->material.shininess.r, 0.1f, 360.0f, NULL, ImGuiSliderFlags_Logarithmic);
    
    
    ImGui::InputText("Model", &model_path);
    ImGui::InputText("Texture", &texture_path);
    if (ImGui::Button("Add Model"))
    {
        Model* cModel = nullptr;
        try {
            cModel = new Model("models/" + model_path, "textures/" + texture_path);
            GenerateUUID(cModel, 8, true);
            loadModel(cModel);
            createTextureImage(cModel);
            createTextureImageView(cModel);
            createTextureSampler(cModel);
            createVertexBuffer(cModel);
            createIndexBuffer(cModel);
            createUniformBuffers(cModel);
            createDescriptorPool(cModel);
            createDescriptorSets(cModel);
            addtoScene(cModel);
            statsFaces += cModel->statsFaces;
        }
        catch(const std::exception& e){
            delete cModel;
            std::cerr << e.what() << std::endl;
            
        }
    }
    if (ImGui::Button("Delete Selected Model"))
    {
        std::vector<Model*> newScene;
        for (size_t i = 0; i < scene.size(); i++)
        {
            if (strcmp(scene[i]->UUID.c_str(), mCurrentSelectedModel->UUID.c_str()))
            {
                newScene.push_back(scene[i]);
            }
    
        }
        shouldDestroy = true;
        
        scene.resize(0);
        for (size_t i = 0 ; i < newScene.size(); i++)
        {
            scene.push_back(newScene[i]);
        }
        statsFaces -= mCurrentSelectedModel->statsFaces;
    }
     
    ImGui::Checkbox("AddModel", &addModel);
    if (addModel)
    {
        if (ImGui::Button("Refresh"))
        {
            traceDir("res/models/", "res/textures/");
        }
        if (ImGui::BeginListBox("Models", ImVec2(200, 100))) {
            static size_t item_current_idx = 0;
            for (size_t i = 0; i < model_paths.size(); ++i)
            {
                const bool isSelected = (item_current_idx == i);
                if (ImGui::Selectable(model_paths[i].c_str(), isSelected))
                {
                    item_current_idx = i;
                    model_path = model_paths[i];
                }
                if (isSelected)
                    ImGui::SetItemDefaultFocus();
            }
            ImGui::EndListBox();
        }

        if (ImGui::BeginListBox("Textures", ImVec2(200, 100))) {
            static size_t item_current_idx2 = 0;
            for (size_t i = 0; i < texture_paths.size(); ++i)
            {
                const bool isSelected = (item_current_idx2 == i);
                if (ImGui::Selectable(texture_paths[i].c_str(), isSelected))
                {
                    item_current_idx2 = i;
                    texture_path = texture_paths[i];
                }
                if (isSelected)
                    ImGui::SetItemDefaultFocus();
            }

            ImGui::EndListBox();
        }

    }
    
    
    
    ImGui::End();

    ImGui::SetNextWindowPos(ImVec2(swapChainExtent.width - 735 , 0));
    ImGui::Begin("Scene", 0, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize);
    
    if (ImGui::BeginTable("CameraPos", 4))
    {
        ImGui::TableSetupColumn("POS");
        ImGui::TableSetupColumn("X");
        ImGui::TableSetupColumn("Y");
        ImGui::TableSetupColumn("Z");
        ImGui::TableHeadersRow();
        
        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::Text("Camera Position");
        ImGui::TableNextColumn();
        ImGui::Text("%.5f", camera->Position.x);
        ImGui::TableNextColumn();
        ImGui::Text("%.5f", camera->Position.y);
        ImGui::TableNextColumn();
        ImGui::Text("%.5f", camera->Position.z);
        
        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::Text("Orientation");
        ImGui::TableNextColumn();
        ImGui::Text("%.5f", camera->Orientation.x);
        ImGui::TableNextColumn();
        ImGui::Text("%.5f", camera->Orientation.y);
        ImGui::TableNextColumn();
        ImGui::Text("%.5f", camera->Orientation.z);
        ImGui::EndTable();
        
    }

    if (ImGui::BeginTable("Scene Details", 3))
    {
        ImGui::TableSetupColumn("MODEL");
        ImGui::TableSetupColumn("ID");
        ImGui::TableSetupColumn("TRIANGLES");
        
        ImGui::TableHeadersRow();

        for (size_t i = 0; i < scene.size(); ++i)
        {
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::Text(scene[i]->MODEL_PATH.c_str());
            ImGui::TableNextColumn();
            ImGui::Text(scene[i]->UUID.c_str());
            ImGui::TableNextColumn();
            ImGui::Text("%d", scene[i]->statsFaces);
            
        }
        ImGui::EndTable();
    }

    
    
    ImGui::End();
    if (true)
    {
        
        if (glfwGetKey(window, GLFW_KEY_T) == GLFW_PRESS)
        {
            mCurrentGizmoOperation = ImGuizmo::TRANSLATE;
        }
        if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS)
        {
            mCurrentGizmoOperation = ImGuizmo::ROTATE;
        }
        if (glfwGetKey(window, GLFW_KEY_Y) == GLFW_PRESS)
        {
            mCurrentGizmoOperation = ImGuizmo::SCALE;
        }
        if (glfwGetKey(window, GLFW_KEY_U) == GLFW_PRESS)
        {
            mCurrentGizmoOperation = ImGuizmo::UNIVERSAL;
        }
        //if (glfwGetKey(window, GLFW_KEY_I) == GLFW_PRESS)
        //{
        //    mCurrentSelectedModel = nullptr;
        //}
        if (false && glfwGetKey(window, GLFW_KEY_M) == GLFW_PRESS ) // this is the worst code i have ever written // currently doesn't work
        {
            
            for (size_t i = 0; i < scene.size(); ++i)
            {
                scene[i]->collider.center = scene[i]->translationVec;
                scene[i]->collider.depth = 80.0f;
                scene[i]->collider.width = 50.0f;
                scene[i]->collider.height = 100.0f;

                bool collide = scene[i]->collider.rayIntersects(camera->Position, camera->Orientation);

                if (collide)
                {
                    mCurrentSelectedModel = scene[i];
                }
                else {
                    printf("False!");
                }
            }
        }
        
        ImGuizmo::SetOrthographic(false);
        ImGuizmo::SetDrawlist(ImGui::GetForegroundDrawList());

        ImGuizmo::SetRect(0, 0, swapChainExtent.width, swapChainExtent.height);
        if (!lightTranslateEnable && mCurrentSelectedModel != nullptr)
        {
            ImGuizmo::Manipulate(glm::value_ptr(camera->view), glm::value_ptr(camera->proj), mCurrentGizmoOperation, mCurrentGizmoMode, glm::value_ptr(mCurrentSelectedModel->transform));

            if (ImGuizmo::IsUsing())
            {
                glm::vec3 translation, rotation, scale;
                DecomposeTransform(mCurrentSelectedModel->transform, translation, rotation, scale);
                mCurrentSelectedModel->translationVec = translation;
                mCurrentSelectedModel->rotationVec = rotation;
                mCurrentSelectedModel->scaleVec = scale;
            }
        }
        else {
            ImGuizmo::Manipulate(glm::value_ptr(camera->view), glm::value_ptr(camera->proj), mCurrentGizmoOperation, mCurrentGizmoMode, glm::value_ptr(camera->lightMat));

            if (ImGuizmo::IsUsing())
            {
                glm::vec3 translation, rotation, scale;
                DecomposeTransform(camera->lightMat, translation, rotation, scale);
                camera->lightPos = translation;
                
            }
        }
        
    }
    
    ImGui::Render();
}

void Engine::createSyncObjects()
{
    imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, &imageAvailableSemaphores[i]) != VK_SUCCESS || vkCreateSemaphore(device, &semaphoreInfo, nullptr, &renderFinishedSemaphores[i]) != VK_SUCCESS || vkCreateFence(device, &fenceInfo, nullptr, &inFlightFences[i]) != VK_SUCCESS) {
            throw std::runtime_error("ERROR: failed to create synchronization objects for a frame!");
        }
    }
}

void Engine::drawFrame()
{
    vkWaitForFences(device, 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);


    if (shouldDestroy && scene.size())
    {
        vkQueueWaitIdle(graphicsQueue);
        cleanUpModel(mCurrentSelectedModel);
        mCurrentSelectedModel = scene.back();
    }
    if (shouldUpdatePipeline)
    {
        vkQueueWaitIdle(graphicsQueue);
        vkDestroyPipeline(device, graphicsPipeline, nullptr);
        vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
        createGraphicsPipeline();
        shouldUpdatePipeline = false;
    }
    

    uint32_t imageIndex;
    VkResult result = vkAcquireNextImageKHR(device, swapChain, UINT64_MAX, imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);

    if (result == VK_ERROR_OUT_OF_DATE_KHR)
    {
        recreateSwapChain();
        return;
    }
    else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        throw std::runtime_error("ERROR: failed to acquire swap chain image!");
    }

    

    for (size_t i = 0; i < scene.size(); i++)
    {
        updateUniformBuffers(currentFrame, scene[i]);
    }

    
    
    
    recordCommandBuffer(commandBuffers[currentFrame], imageIndex);
    

    vkResetFences(device, 1, &inFlightFences[currentFrame]);
    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkSemaphore waitSemaphores[] = { imageAvailableSemaphores[currentFrame] };
    VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;

    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffers[currentFrame];

    VkSemaphore signalSemaphores[] = { renderFinishedSemaphores[currentFrame] };
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    if (vkQueueSubmit(graphicsQueue, 1, &submitInfo, inFlightFences[currentFrame]) != VK_SUCCESS) {
        throw std::runtime_error("ERROR: failed to submit draw command buffer!");
    }

    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;

    VkSwapchainKHR swapChains[] = { swapChain };
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;

    presentInfo.pImageIndices = &imageIndex;

    result = vkQueuePresentKHR(presentQueue, &presentInfo);

    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || framebufferResized)
    {
        framebufferResized = false;
        recreateSwapChain();
    } 
    else if (result  != VK_SUCCESS) {
        throw std::runtime_error("ERROR: failed to present swap chain image!");
    } 

    currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;

}

void Engine::createVertexBuffer(Model* model) // investigage if Vertexbuffermemory must be unique
{
    VkDeviceSize bufferSize = sizeof(model->vertices[0]) * model->vertices.size();

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

    void* data;
    vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &data);
    memcpy(data, model->vertices.data(), (size_t)bufferSize);
    vkUnmapMemory(device, stagingBufferMemory);

    createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, model->vertexBuffer, model->vertexBufferMemory);

    copyBuffer(stagingBuffer, model->vertexBuffer, bufferSize);

    vkDestroyBuffer(device, stagingBuffer, nullptr);
    vkFreeMemory(device, stagingBufferMemory, nullptr);
}

void Engine::createIndexBuffer(Model* model)
{
    VkDeviceSize bufferSize = sizeof(model->indices[0]) * model->indices.size();

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);
    
    void* data;
    vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &data);
    memcpy(data, model->indices.data(), (size_t)bufferSize);
    vkUnmapMemory(device, stagingBufferMemory);

    createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, model->indexBuffer, model->indexBufferMemory);
    copyBuffer(stagingBuffer, model->indexBuffer, bufferSize);
    vkDestroyBuffer(device, stagingBuffer, nullptr);
    vkFreeMemory(device, stagingBufferMemory, nullptr);
}

void Engine::createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory)
{
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateBuffer(device, &bufferInfo, nullptr, &buffer) != VK_SUCCESS) {
        throw std::runtime_error("ERROR:failed to create buffer!");
    }

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(device, buffer, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

    if (vkAllocateMemory(device, &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS) {
        throw std::runtime_error("ERROR: failed to allocate buffer memory!");
    }

    vkBindBufferMemory(device, buffer, bufferMemory, 0);
}

void Engine::copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size)
{
    VkCommandBuffer commandBuffer = beginSingleTimeCommands();

    VkBufferCopy copyRegion{};
    copyRegion.size = size;
    vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

    endSingleTimeCommands(commandBuffer);
}


    

uint32_t Engine::findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties)
{
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++)
    {
        if (typeFilter & (1 << i) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
            return i;
    }
    throw std::runtime_error("ERROR: failed to find suitable memory type!");
}

void Engine::createDescriptorSetLayout()
{
    VkDescriptorSetLayoutBinding uboLayoutBinding{};
    uboLayoutBinding.binding = 0;
    uboLayoutBinding.descriptorCount = 1;
    uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uboLayoutBinding.pImmutableSamplers = nullptr;
    uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    VkDescriptorSetLayoutBinding samplerLayoutBinding{};
    samplerLayoutBinding.binding = 1;
    samplerLayoutBinding.descriptorCount = 1;
    samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    samplerLayoutBinding.pImmutableSamplers = nullptr;
    samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    std::array<VkDescriptorSetLayoutBinding, 2> bindings = { uboLayoutBinding, samplerLayoutBinding };
    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
    layoutInfo.pBindings = bindings.data();

    if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS)
        throw std::runtime_error("ERROR: failed to create descriptor set layout!");
}

void Engine::createUniformBuffers(Model* model)
{
    VkDeviceSize bufferSize = sizeof(UniformBufferObject);

    model->uniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);
    model->uniformBuffersMemory.resize(MAX_FRAMES_IN_FLIGHT);
    model->uniformBuffersMapped.resize(MAX_FRAMES_IN_FLIGHT);

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, model->uniformBuffers[i], model->uniformBuffersMemory[i]);

        vkMapMemory(device, model->uniformBuffersMemory[i], 0, bufferSize, 0, &model->uniformBuffersMapped[i]);
    }
}

void Engine::updateUniformBuffers(uint32_t currentImage, Model* model)
{
    
    camera->UpdateMatrices(FOV, model);

    UniformBufferObject ubo{};
    ubo.transform = model->transform;
    ubo.view = camera->view;
    ubo.proj = camera->proj;
    ubo.cameraPos = camera->Position;
    ubo.lightPos = camera->lightPos;
    ubo.lightColor = camera->lightColor;
    ubo.material = model->material;
    ubo.proj[1][1] *= -1;

    memcpy(model->uniformBuffersMapped[currentImage], &ubo, sizeof(ubo));
}

void Engine::createDescriptorPool(Model* model)
{
    std::array<VkDescriptorPoolSize, 2> poolSizes{};
    poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSizes[0].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
    poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSizes[1].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT) * 2;

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
    poolInfo.pPoolSizes = poolSizes.data();
    poolInfo.maxSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT) * 2;

    if (vkCreateDescriptorPool(device, &poolInfo, nullptr, &model->descriptorPool) != VK_SUCCESS) {
        throw std::runtime_error("failed to create descriptor pool!");
    }
}


void Engine::createImGuiDP()
{
    std::array<VkDescriptorPoolSize, 2> poolSizes{};
    poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSizes[0].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
    poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSizes[1].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT) * 2;

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
    poolInfo.pPoolSizes = poolSizes.data();
    poolInfo.maxSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT) * 2;

    if (vkCreateDescriptorPool(device, &poolInfo, nullptr, &imGuiDP) != VK_SUCCESS) {
        throw std::runtime_error("failed to create descriptor pool!");
    }
}

void Engine::createDescriptorSets(Model* model)
{
    std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, descriptorSetLayout);
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = model->descriptorPool;
    allocInfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
    allocInfo.pSetLayouts = layouts.data();

    model->descriptorSets.resize(MAX_FRAMES_IN_FLIGHT);
    if (vkAllocateDescriptorSets(device, &allocInfo, model->descriptorSets.data()) != VK_SUCCESS) {
        throw std::runtime_error("ERROR: failed to allocate descriptor sets!");
    }

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        VkDescriptorBufferInfo bufferInfo{};
        bufferInfo.buffer = model->uniformBuffers[i];
        bufferInfo.offset = 0;
        bufferInfo.range = sizeof(UniformBufferObject);

        VkDescriptorImageInfo imageInfo{};
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageInfo.imageView = model->textureImageView;
        imageInfo.sampler = model->textureSampler;

        std::array<VkWriteDescriptorSet, 2> descriptorWrites{};

        descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[0].dstSet = model->descriptorSets[i];
        descriptorWrites[0].dstBinding = 0;
        descriptorWrites[0].dstArrayElement = 0;
        descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorWrites[0].descriptorCount = 1;
        descriptorWrites[0].pBufferInfo = &bufferInfo;

        descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[1].dstSet = model->descriptorSets[i];
        descriptorWrites[1].dstBinding = 1;
        descriptorWrites[1].dstArrayElement = 0;
        descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        descriptorWrites[1].descriptorCount = 1;
        descriptorWrites[1].pImageInfo = &imageInfo;

        vkUpdateDescriptorSets(device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
    }
}

VkCommandBuffer Engine::beginSingleTimeCommands() {
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = commandPool;
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer;
    vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer);

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(commandBuffer, &beginInfo);

    return commandBuffer;
}

void Engine::endSingleTimeCommands(VkCommandBuffer commandBuffer) {
    vkEndCommandBuffer(commandBuffer);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(graphicsQueue);

    vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
}

void Engine::createTextureImage(Model* model)
{
    int texWidth, texHeight, texChannels;
    stbi_uc* pixels = stbi_load((model->baseDir + model->TEXTURE_PATH).c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
    VkDeviceSize imageSize = texWidth * texHeight * 4; // 4 channels
    model->mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(texWidth, texHeight)))) + 1;

    if (!pixels)
    {
        throw std::runtime_error("ERROR failed to load texture image!");
    }

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;

    createBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);
    
    void* data;
    vkMapMemory(device, stagingBufferMemory, 0, imageSize, 0, &data);
    memcpy(data, pixels, static_cast<size_t>(imageSize));
    vkUnmapMemory(device, stagingBufferMemory);
    stbi_image_free(pixels);
    
    createImage(texWidth, texHeight, model->mipLevels, VK_SAMPLE_COUNT_1_BIT, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, model->textureImage, model->textureImageMemory);
    transitionImageLayout(model->textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, model->mipLevels);
    copyBufferToImage(stagingBuffer, model->textureImage, static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight));
    generateMipmaps(model->textureImage, VK_FORMAT_R8G8B8A8_SRGB, texWidth, texHeight, model->mipLevels);

    vkDestroyBuffer(device, stagingBuffer, nullptr);
    vkFreeMemory(device, stagingBufferMemory, nullptr);
}

void Engine::createColorResources() {
    VkFormat colorFormat = swapChainImageFormat;

    createImage(swapChainExtent.width, swapChainExtent.height, 1, msaaSamples, colorFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, colorImage, colorImageMemory);
    colorImageView = createImageView(colorImage, colorFormat, VK_IMAGE_ASPECT_COLOR_BIT, 1);
}

void Engine::generateMipmaps(VkImage image, VkFormat imageFormat,int32_t texWidth, int32_t texHeight, uint32_t mipLevels)
{
    VkFormatProperties formatProperties;
    vkGetPhysicalDeviceFormatProperties(physicalDevice, imageFormat, &formatProperties);

    if (!(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT)) {
        throw std::runtime_error("texture image format does not support linear blitting!");
    }

    VkCommandBuffer commandBuffer = beginSingleTimeCommands();

    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.image = image;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;
    barrier.subresourceRange.levelCount = 1;

    int32_t mipWidth = texWidth;
    int32_t mipHeight = texHeight;

    for (uint32_t i = 1; i < mipLevels; i++) {
        barrier.subresourceRange.baseMipLevel = i - 1;
        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

        vkCmdPipelineBarrier(commandBuffer,
            VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
            0, nullptr,
            0, nullptr,
            1, &barrier);

        VkImageBlit blit{};
        blit.srcOffsets[0] = { 0, 0, 0 };
        blit.srcOffsets[1] = { mipWidth, mipHeight, 1 };
        blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        blit.srcSubresource.mipLevel = i - 1;
        blit.srcSubresource.baseArrayLayer = 0;
        blit.srcSubresource.layerCount = 1;
        blit.dstOffsets[0] = { 0, 0, 0 };
        blit.dstOffsets[1] = { mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1 };
        blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        blit.dstSubresource.mipLevel = i;
        blit.dstSubresource.baseArrayLayer = 0;
        blit.dstSubresource.layerCount = 1;

        vkCmdBlitImage(commandBuffer,
            image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            1, &blit,
            VK_FILTER_LINEAR);

        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);

        if (mipWidth > 1) mipWidth /= 2;
        if (mipHeight > 1) mipHeight /= 2;
    }

    barrier.subresourceRange.baseMipLevel = mipLevels - 1;
    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    vkCmdPipelineBarrier(commandBuffer,VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,0, nullptr, 0, nullptr, 1, &barrier);

    endSingleTimeCommands(commandBuffer);
}

void Engine::createImage(uint32_t width, uint32_t height, uint32_t mipLevels , VkSampleCountFlagBits numSamples,VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory)
{
    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = static_cast<uint32_t>(width);
    imageInfo.extent.height = static_cast<uint32_t>(height);
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = mipLevels;
    imageInfo.arrayLayers = 1;
    imageInfo.format = format;
    imageInfo.tiling = tiling;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = usage;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    imageInfo.samples = numSamples;
    imageInfo.flags = 0;

    if (vkCreateImage(device, &imageInfo, nullptr, &image) != VK_SUCCESS)
        throw std::runtime_error("ERROR: failed to create image!");

    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(device, image, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    if (vkAllocateMemory(device, &allocInfo, nullptr, &imageMemory) != VK_SUCCESS)
        throw std::runtime_error("ERROR: failed to allocate iamge memory!");

    vkBindImageMemory(device, image, imageMemory, 0);
}

void Engine::transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t mipLevels) 
{
    VkCommandBuffer commandBuffer = beginSingleTimeCommands();

    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = oldLayout;
    barrier.newLayout = newLayout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = image;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = mipLevels;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;

    VkPipelineStageFlags sourceStage;
    VkPipelineStageFlags destinationStage;

    if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

        sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    }
    else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    }
    else {
        throw std::invalid_argument("ERROR: unsupported layout transition!");
    }

    vkCmdPipelineBarrier(
        commandBuffer,
        sourceStage, destinationStage,
        0,
        0, nullptr,
        0, nullptr,
        1, &barrier
    );

    endSingleTimeCommands(commandBuffer);
}

void Engine::copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height) 
{
    VkCommandBuffer commandBuffer = beginSingleTimeCommands();

    VkBufferImageCopy region{};
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;
    region.imageOffset = { 0, 0, 0 };
    region.imageExtent = {
        width,
        height,
        1
    };

    vkCmdCopyBufferToImage(commandBuffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

    endSingleTimeCommands(commandBuffer);
}

void Engine::createTextureImageView(Model* model)
{
    model->textureImageView = createImageView(model->textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT, model->mipLevels);

}

void Engine::createTextureSampler(Model* model)
{
    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.anisotropyEnable = VK_TRUE;
    
    VkPhysicalDeviceProperties properties{};
    vkGetPhysicalDeviceProperties(physicalDevice, &properties);
    
    samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.mipLodBias = 0.0f;
    samplerInfo.minLod = 0.0f;
    samplerInfo.maxLod = 0.0f;

    if (vkCreateSampler(device, &samplerInfo, nullptr, &model->textureSampler) != VK_SUCCESS)
    {
        throw std::runtime_error("ERROR: failed to create texture sampler!");
    }
}

VkImageView Engine::createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, uint32_t mipLevels)
{
    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = image;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = format;
    viewInfo.subresourceRange.aspectMask = aspectFlags;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.levelCount = mipLevels;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.layerCount = 1;

    VkImageView imageView;

    if (vkCreateImageView(device, &viewInfo, nullptr, &imageView) != VK_SUCCESS)
    {
        throw std::runtime_error("ERROR: failed to create texture image view!");
    }
    return imageView;
}

void Engine::createDepthResources() {
    VkFormat depthFormat = findDepthFormat();

    createImage(swapChainExtent.width, swapChainExtent.height, 1, msaaSamples, depthFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, depthImage, depthImageMemory);
    depthImageView = createImageView(depthImage, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT, 1);
}

VkFormat Engine::findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features)
{
    for (VkFormat format : candidates)
    {
        VkFormatProperties props;
        vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &props);
        if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features)
        {
            return format;
        } else if(tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features)
        {
            return format;
        }
    }
    throw std::runtime_error("ERROR: failed to find supported format!");
}

VkFormat Engine::findDepthFormat()
{
    return findSupportedFormat({ VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT }, VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
}

/*
bool Engine::hasStencilComponent(VkFormat format)
{
    return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D32_SFLOAT_S8_UINT;
}
*/


void Engine::loadFile(std::string filename)
{
    std::ifstream f{"res/data/" + filename};
    json j = json::parse(f);

    for (size_t i = 0; i < sceneSize; ++i)
    {
        
        scene[i]->translationVec = glm::vec3(j["Section2"][scene[i]->UUID]["TRANSLATION"][0][0], j["Section2"][scene[i]->UUID]["TRANSLATION"][0][1], j["Section2"][scene[i]->UUID]["TRANSLATION"][0][2]);
        scene[i]->rotationVec = glm::vec3(j["Section2"][scene[i]->UUID]["ROTATION"][0][0], j["Section2"][scene[i]->UUID]["ROTATION"][0][1], j["Section2"][scene[i]->UUID]["ROTATION"][0][2]);
        scene[i]->scaleVec = glm::vec3(j["Section2"][scene[i]->UUID]["SCALE"][0][0], j["Section2"][scene[i]->UUID]["SCALE"][0][1], j["Section2"][scene[i]->UUID]["SCALE"][0][2]);

    }
    camera->Position = glm::vec3(j["Section3"]["CameraInfo"]["CameraPos"][0][0], j["Section3"]["CameraInfo"]["CameraPos"][0][1], j["Section3"]["CameraInfo"]["CameraPos"][0][2]);
    camera->Orientation = glm::vec3(j["Section3"]["CameraInfo"]["CameraOrientation"][0][0], j["Section3"]["CameraInfo"]["CameraOrientation"][0][1], j["Section3"]["CameraInfo"]["CameraOrientation"][0][2]);
    camera->lightPos = glm::vec3(j["Section3"]["LightInfo"]["LightPos"][0][0], j["Section3"]["LightInfo"]["LightPos"][0][1], j["Section3"]["LightInfo"]["LightPos"][0][2]);
    camera->lightColor = glm::vec3(j["Section3"]["LightInfo"]["LightColor"][0][0], j["Section3"]["LightInfo"]["LightColor"][0][1], j["Section3"]["LightInfo"]["LightColor"][0][2]);
}

void Engine::writeToFile(std::vector<Model*> scene)
{
    bool willNotReturn = false;
    std::vector<std::string> ids;
    ids.resize(scene.size());
    for (size_t i = 0; i < scene.size(); ++i)
    {
        
        if (scene[i]->UUID.empty())
        {
            std::string wrn = std::string("Empty UUID cannot serialize model with index: ") + std::to_string(i) + "\n";

            tlog::warning(wrn);
            willNotReturn = true;
            
        }
        for (auto& id : ids)
        {
            if (!strcmp(scene[i]->UUID.c_str(), id.c_str()))
            {
                std::string wrn = std::string("UUID of model with index: ") + std::to_string(i) + " already exists\n";
                tlog::warning(wrn);
                willNotReturn = true;
            }
            
        }
        ids[i] = scene[i]->UUID;
        
    }
    if (willNotReturn)
        return;
    std::ofstream f("res/data/" + scene_path);
    json j;

    for (size_t i = 0; i < scene.size(); ++i)
    {
        
        j["Section1"]["ModelPaths"].push_back(json::string_t(scene[i]->MODEL_PATH));
        j["Section1"]["TexturePaths"].push_back(json::string_t(scene[i]->TEXTURE_PATH));
        j["Section1"]["UUIDs"].push_back(json::string_t(scene[i]->UUID));
        j["Section2"][scene[i]->UUID]["TRANSLATION"].push_back({ json::number_float_t(scene[i]->translationVec.x), json::number_float_t(scene[i]->translationVec.y), json::number_float_t(scene[i]->translationVec.z) });
        j["Section2"][scene[i]->UUID]["ROTATION"].push_back({ json::number_float_t(scene[i]->rotationVec.x), json::number_float_t(scene[i]->rotationVec.y), json::number_float_t(scene[i]->rotationVec.z) });
        j["Section2"][scene[i]->UUID]["SCALE"].push_back({ json::number_float_t(scene[i]->scaleVec.x), json::number_float_t(scene[i]->scaleVec.y), json::number_float_t(scene[i]->scaleVec.z) });


    }
    j["Section3"]["CameraInfo"]["CameraPos"].push_back({ json::number_float_t(camera->Position.x), json::number_float_t(camera->Position.y), json::number_float_t(camera->Position.z) });
    j["Section3"]["CameraInfo"]["CameraOrientation"].push_back({ json::number_float_t(camera->Orientation.x), json::number_float_t(camera->Orientation.y), json::number_float_t(camera->Orientation.z) });
    j["Section3"]["LightInfo"]["LightPos"].push_back({ json::number_float_t(camera->lightPos.x), json::number_float_t(camera->lightPos.y), json::number_float_t(camera->lightPos.z) });
    j["Section3"]["LightInfo"]["LightColor"].push_back({ json::number_float_t(camera->lightColor.x), json::number_float_t(camera->lightColor.y), json::number_float_t(camera->lightColor.z) });
    
    f << std::setw(4) << j << std::endl;
}

void Engine::loadModel(Model* model) {
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string err;

    if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &err, (model->baseDir + model->MODEL_PATH).c_str(), "res/models/")) {
        throw std::runtime_error(err);
    }

    std::unordered_map<Vertex, uint32_t> uniqueVertices{};

    static const int N = static_cast<const int>(shapes.size());
    auto progress = tlog::progress(N);

    int prog = 0;
    for (const auto& shape : shapes) {
        for (const auto& index : shape.mesh.indices) {
            Vertex vertex{};
            
            vertex.pos = {
                attrib.vertices[3 * index.vertex_index + 0],
                attrib.vertices[3 * index.vertex_index + 1],
                attrib.vertices[3 * index.vertex_index + 2]
            };
            
            vertex.normal = {
                attrib.normals[3 * index.normal_index + 0],
		        attrib.normals[3 * index.normal_index + 1],
			    attrib.normals[3 * index.normal_index + 2]
            };

            vertex.texCoord = {
                attrib.texcoords[2 * index.texcoord_index + 0],
                1.0f - attrib.texcoords[2 * index.texcoord_index + 1]
            };

            vertex.color = { 1.0f, 1.0f, 1.0f };

            if (uniqueVertices.count(vertex) == 0) {
                uniqueVertices[vertex] = static_cast<uint32_t>(model->vertices.size());
                model->vertices.push_back(vertex);
            }
            
            model->indices.push_back(uniqueVertices[vertex]);
            
        }
        prog++;
        std::this_thread::sleep_for(std::chrono::milliseconds{100});
        progress.update(prog);
    }

    model->material.ambient = glm::vec3(materials[0].ambient[0], materials[0].ambient[1], materials[0].ambient[2]);
    model->material.diffuse = glm::vec3(materials[0].diffuse[0], materials[0].diffuse[1], materials[0].diffuse[2]);
    model->material.specular = glm::vec3(materials[0].specular[0], materials[0].specular[1], materials[0].specular[2]);
    model->material.shininess = glm::vec3(materials[0].shininess, 0.0f, 0.0f);
    
    
    model->statsFaces = static_cast<int>((model->indices.size()/3));
    printf("\n");
    tlog::none();
    tlog::success();
    printf("Material count: %d \n", static_cast<int>(materials.size()));
}

void Engine::traceDir(std::string modelDirectory, std::string textureDirectory)
{
    
    model_paths.resize(0);
    texture_paths.resize(0);
    for (const auto& entry : std::filesystem::directory_iterator(modelDirectory))
    {
        std::filesystem::path outfilename = entry.path();
        if (outfilename.extension() == ".obj")
        {
            std::string outfilename_str = outfilename.string();

            model_paths.push_back(clear_slash(outfilename_str));
        }
    }
    for (const auto& entry : std::filesystem::directory_iterator(textureDirectory))
    {
        std::filesystem::path outfilename = entry.path();
        if (outfilename.extension() == ".png" || outfilename.extension() == ".jpg" || outfilename.extension() == "jpeg")
        {
            std::string outfilename_str = outfilename.string();
    
            texture_paths.push_back(clear_slash(outfilename_str));
        }
    }
}

void Engine::traceScenesDir(std::string sceneDirectory)
{
    
    scene_paths.resize(0);
    for (const auto& entry : std::filesystem::directory_iterator(sceneDirectory))
    {
        std::filesystem::path outfilename = entry.path();
        if (outfilename.extension() == ".json")
        {
            std::string outfilename_str = outfilename.string();
            scene_paths.push_back(clear_slash(outfilename_str));
        }
    }
}


VkSampleCountFlagBits Engine::getMaxUsableSampleCount()
{
    VkPhysicalDeviceProperties physicalDeviceProperties;
    vkGetPhysicalDeviceProperties(physicalDevice, &physicalDeviceProperties);

    VkSampleCountFlags counts = physicalDeviceProperties.limits.framebufferColorSampleCounts & physicalDeviceProperties.limits.framebufferDepthSampleCounts;
    if (counts & VK_SAMPLE_COUNT_64_BIT) { return VK_SAMPLE_COUNT_64_BIT; }
    if (counts & VK_SAMPLE_COUNT_32_BIT) { return VK_SAMPLE_COUNT_32_BIT; }
    if (counts & VK_SAMPLE_COUNT_16_BIT) { return VK_SAMPLE_COUNT_16_BIT; }
    if (counts & VK_SAMPLE_COUNT_8_BIT) { return VK_SAMPLE_COUNT_8_BIT; }
    if (counts & VK_SAMPLE_COUNT_4_BIT) { return VK_SAMPLE_COUNT_4_BIT; }
    if (counts & VK_SAMPLE_COUNT_2_BIT) { return VK_SAMPLE_COUNT_2_BIT; }

    return VK_SAMPLE_COUNT_1_BIT;
}

void Engine::drawWindowTitle()
{
    double currentTime = glfwGetTime();
    double delta = currentTime - lastTime;
    nbFrames++;
    double fps = 60.0;
    if (delta >= 1.0)
    {
        double timeToDraw = 1000.0 / double(nbFrames);
        double fps = double(nbFrames) / delta;
        std::stringstream ss;
        ss << TITLE << " " << VERSION << " [" << fps << " FPS]" << " [" << timeToDraw << "ms  Frametime ]" << "[ TRIANGLES COUNT: " << statsFaces << " ]";

        glfwSetWindowTitle(window, ss.str().c_str());

        nbFrames = 0;
        lastTime = currentTime;
    }
    
}

void Engine::loadScene()
{
    scene.resize(0);
    std::ifstream f{"res/data/" + scene_path};
    json j = json::parse(f);
    sceneSize = j["Section1"]["UUIDs"].size();
    for (size_t i = 0; i < sceneSize; ++i)
    {
        std::string s1 = j["Section1"]["ModelPaths"][i];
        s1.erase(remove(s1.begin(), s1.end(), '\"'), s1.end());
        std::string s2 = j["Section1"]["TexturePaths"][i];
        s2.erase(remove(s2.begin(), s2.end(), '\"'), s2.end());
        Model* cModel = new Model(s1.c_str(), s2.c_str());
        cModel->UUID = j["Section1"]["UUIDs"][i];
        loadModel(cModel);
        addtoScene(cModel);
        statsFaces += cModel->statsFaces;
    }
}

void Engine::addtoScene(Model* model)
{
    
    scene.push_back(model);
}

void Engine::cleanUpModel(Model* model)
{
    //vkDeviceWaitIdle(device);
    
    for (auto& ubo : model->uniformBuffers)
        vkDestroyBuffer(device, ubo, nullptr);
    for (auto& mem : model->uniformBuffersMemory)
        vkFreeMemory(device, mem, nullptr);
    vkDestroyDescriptorPool(device, model->descriptorPool, nullptr);
    vkDestroySampler(device, model->textureSampler, nullptr);
    vkDestroyImageView(device, model->textureImageView, nullptr);
    vkDestroyImage(device, model->textureImage, nullptr);
    vkFreeMemory(device, model->textureImageMemory, nullptr);
    vkDestroyBuffer(device, model->vertexBuffer, nullptr);

    vkFreeMemory(device, model->vertexBufferMemory, nullptr);
    vkDestroyBuffer(device, model->indexBuffer, nullptr);
    vkFreeMemory(device, model->indexBufferMemory, nullptr);
    free(model);

    shouldDestroy = false;
}

void Engine::cleanup()
{
    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    
    free(camera);

    cleanupSwapChain();

    vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);

    vkDestroyDescriptorPool(device, imGuiDP, nullptr);

    for (size_t i = 0; i < scene.size(); ++i)
    {
        for (auto& ubo : scene[i]->uniformBuffers)
            vkDestroyBuffer(device, ubo, nullptr);
        for (auto& mem : scene[i]->uniformBuffersMemory)
            vkFreeMemory(device, mem, nullptr);
        vkDestroyDescriptorPool(device, scene[i]->descriptorPool, nullptr);
        vkDestroySampler(device, scene[i]->textureSampler, nullptr);
        vkDestroyImageView(device, scene[i]->textureImageView, nullptr);
        vkDestroyImage(device, scene[i]->textureImage, nullptr);
        vkFreeMemory(device, scene[i]->textureImageMemory, nullptr);
        vkDestroyBuffer(device, scene[i]->vertexBuffer, nullptr);
        vkFreeMemory(device, scene[i]->vertexBufferMemory, nullptr);
        vkDestroyBuffer(device, scene[i]->indexBuffer, nullptr);
        vkFreeMemory(device, scene[i]->indexBufferMemory, nullptr);
        free(scene[i]);
    }
    
    vkDestroyPipeline(device, graphicsPipeline, nullptr);
    vkDestroyPipelineLayout(device, pipelineLayout, nullptr);

    vkDestroyRenderPass(device, renderPass, nullptr);

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        vkDestroySemaphore(device, renderFinishedSemaphores[i], nullptr);
        vkDestroySemaphore(device, imageAvailableSemaphores[i], nullptr);
        vkDestroyFence(device, inFlightFences[i], nullptr);
    }

    vkDestroyCommandPool(device, commandPool, nullptr);

    vkDestroyDevice(device, nullptr);

    if (enableValidationLayers) {
        DestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
    }

    vkDestroySurfaceKHR(instance, surface, nullptr);
    vkDestroyInstance(instance, nullptr);

    glfwDestroyWindow(window);

    glfwTerminate();
    
}

