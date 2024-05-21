#include "Engine.h"
#include <map>
#include <sstream>


#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>
#include <unordered_map>

#define VK_CHECK(x) do {VkResult err = x;if (err){ printf("Detected Vulkan error %d at %s:%d.\n", int(err), __FILE__, __LINE__); abort();}} while (0)

Engine* retrieveEnginePtr(GLFWwindow * window) {
    return reinterpret_cast<Engine*>(glfwGetWindowUserPointer(window));
}

static void framebufferResizeCallback(GLFWwindow* window, int width, int height)
{
    auto engine = retrieveEnginePtr(window);
    engine->framebufferResized = true;
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
    videoMode = glfwGetVideoMode(monitor);
    glfwSetInputMode(window, GLFW_STICKY_MOUSE_BUTTONS, GLFW_TRUE);
    glfwSetInputMode(window, GLFW_STICKY_KEYS, GLFW_TRUE);
    glfwSetWindowUserPointer(window, (void*)this);
    if (custom_key_callback)
    {
        glfwSetKeyCallback(window, custom_key_callback_wrapper);
    }
    else {
        glfwSetKeyCallback(window, key_callback);
    }
    if (custom_scroll_callback)
    {
        glfwSetScrollCallback(window, custom_scroll_callback_wrapper);
    }
    else{
        //glfwSetKeyCallback(window, scroll_callback); // no default scroll_callback for editor yet
    }
    if (custom_mouse_position_callback)
    {
        glfwSetCursorPosCallback(window, custom_mouse_position_callback_wrapper);
    }
    else {
        // doesn't exist yet
    }
    if (custom_mouse_position_callback)
    {
        glfwSetMouseButtonCallback(window, custom_mouse_button_callback_wrapper);
    }
    else {
        // doesn't exist yet
    }
    glfwSetMouseButtonCallback(window, mouse_callback);
    glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);

}




void Engine::initVulkan()
{
    create_instance(&instance);
    create_surface(&surface, &instance, window);
    pick_physical_device(&physicalDevice, &instance, &surface, &msaaSamples, &RendererName);
    create_logical_device(&device, &physicalDevice, &surface, &graphicsQueue, &presentQueue);
    create_swapchain(&device, &physicalDevice, &swapChainHandle, &surface, window, VSync);
    create_imageviews(&device, &swapChainHandle.imageViews, &swapChainHandle.images, swapChainHandle.imageFormat);
    create_render_pass(&device, &physicalDevice, &renderPass, swapChainHandle.imageFormat, msaaSamples);
    create_descriptor_set_layout(&device, &descriptorSetLayout);
    
    //createPostProcessingDescriptorSets();

    create_command_pool(&device, &physicalDevice, &commandPool, &surface);
    
    create_color_resources(&device, &physicalDevice, &colorImageRes, swapChainHandle.imageFormat, swapChainHandle.extent, msaaSamples);
    create_depth_resources(&device, &physicalDevice, &depthImageRes, swapChainHandle.imageFormat, swapChainHandle.extent, msaaSamples);
    create_framebuffers(&device, &swapChainHandle.framebuffers, swapChainHandle.imageViews, swapChainHandle.extent, renderPass, colorImageRes.imageView, depthImageRes.imageView);
    

    if (firstScene)
        scene_path = "main.json";
    camera = new Camera(swapChainHandle.extent.width, swapChainHandle.extent.height);
    loadScene();
    loadFile(scene_path);

    //createPostProcessingGraphicsPipeline("res/shaders/postProcess_vert.spv", "res/shaders/postProcess_frag.spv");
    loadShaders();


    scene[0]->NORMAL_PATH = std::string("textures/Laocoon-normals.jpg");

    for (size_t i = 0; i < sceneSize; ++i)
    {
        if (!scene[i]->NORMAL_PATH.empty())
        {
            createDescriptorSetLayoutForModel(scene[i]);
        }

        createTextureImage(scene[i]);
        createTextureImageView(scene[i]);
        createTextureSampler(scene[i]);
        createNormal(scene[i]);
        createVertexBuffer(scene[i]);
        createIndexBuffer(scene[i]);
        createUniformBuffers(scene[i]);
        createDescriptorPool(scene[i]);
        createDescriptorSets(scene[i]);
    }
    createImGuiDP();
    
    mCurrentSelectedModel = scene[0];

    create_commandbuffer(&device, &commandBuffers, commandPool);
    createSyncObjects();

    audioMgr = new Audio();

    //physicsEngine = new PhysicsEngine(); // Project on hold
}





void Engine::mainLoop()
{
    
    std::thread fmodThread(fmod_update, window);

    while(!glfwWindowShouldClose(window))
    {
        glfwPollEvents();
        drawWindowTitle();


        if (VSync) // kinda broken // when turning back on performance is lost and noticeable screen tearing occurs
        {
            camera->UpdateInputs(window, customCameraFunction);
            drawFrame();
            continue;
        }
        
        double currentTime = glfwGetTime(); // shitty implementation of dt
        double deltaTime = currentTime - lastTime1;
        
        //std::this_thread::sleep_for(std::chrono::milliseconds(16));
        if (deltaTime >= (1.0/videoMode->refreshRate))
        {   
            camera->UpdateInputs(window, customCameraFunction);
            //physicsEngine->Update(); // project on hold
            
            lastTime1 = currentTime;
        }

        if (custom_main_update)
        {
            custom_main_update();
        }
           
        drawFrame();
    }
    fmodThread.join();
    vkDeviceWaitIdle(device);
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



void Engine::createGraphicsPipelineWrapper(std::string vertShaderPath, std::string fragShaderPath)
{
    createGraphicsPipeline(static_cast<int>(graphicsPipelines.size()), vertShaderPath, fragShaderPath);
}

void Engine::createGraphicsPipeline(int index,std::string vertShaderPath, std::string fragShaderPath){
    auto vertShaderCode = util::readFile(vertShaderPath);
    auto fragShaderCode = util::readFile(fragShaderPath);
    
    //auto tesselationControlShaderCode = util::readFile("res/shaders/tesselation_control.spv");
    //auto tesselationEvalShaderCode = util::readFile("res/shaders/tesselation_eval.spv");

    VkShaderModule vertShaderModule = createShaderModule(vertShaderCode);
    VkShaderModule fragShaderModule = createShaderModule(fragShaderCode);
    //VkShaderModule tesselationControlModule = createShaderModule(tesselationControlShaderCode);
    //VkShaderModule tesselationEvalModule = createShaderModule(tesselationEvalShaderCode);

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

    /*
    VkPipelineShaderStageCreateInfo tesselationControlShaderStageInfo{};
    tesselationControlShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    tesselationControlShaderStageInfo.stage = VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
    tesselationControlShaderStageInfo.module = tesselationControlModule;
    tesselationControlShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo tesselationEvalShaderStageInfo{};
    tesselationEvalShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    tesselationEvalShaderStageInfo.stage = VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
    tesselationEvalShaderStageInfo.module = tesselationEvalModule;
    tesselationEvalShaderStageInfo.pName = "main";
   
    VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, tesselationControlShaderStageInfo, tesselationEvalShaderStageInfo, fragShaderStageInfo};
    */

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
    rasterizer.depthClampEnable = VK_TRUE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = VK_CULL_MODE_BACK_BIT; //VK_CULL_MODE_NONE; used to do this cause i am lazy but VK_CULL_MODE_BACK_BIT IS MORE EFFICIENT
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
    
    pipelineLayouts.resize(pipelineLayouts.size() + 1);
    pipelineLayouts[index] = VkPipelineLayout();

    if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pipelineLayouts[index]) != VK_SUCCESS) {
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
    pipelineInfo.layout = pipelineLayouts[index];
    pipelineInfo.renderPass = renderPass;
    pipelineInfo.subpass = 0;
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

    graphicsPipelines.resize(graphicsPipelines.size() + 1);
    graphicsPipelines[index] = VkPipeline();

    if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipelines[index]) != VK_SUCCESS) {
        throw std::runtime_error("ERROR: failed to create graphics pipeline!");
    }
    //VK_CHECK(vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipelines[index]));

    vkDestroyShaderModule(device, fragShaderModule, nullptr);
    vkDestroyShaderModule(device, vertShaderModule, nullptr);

}

void Engine::createPostProcessingGraphicsPipeline(std::string vertShaderPath, std::string fragShaderPath) {
    auto vertShaderCode = util::readFile(vertShaderPath);
    auto fragShaderCode = util::readFile(fragShaderPath);

    //auto tesselationControlShaderCode = util::readFile("res/shaders/tesselation_control.spv");
    //auto tesselationEvalShaderCode = util::readFile("res/shaders/tesselation_eval.spv");

    VkShaderModule vertShaderModule = createShaderModule(vertShaderCode);
    VkShaderModule fragShaderModule = createShaderModule(fragShaderCode);
    //VkShaderModule tesselationControlModule = createShaderModule(tesselationControlShaderCode);
    //VkShaderModule tesselationEvalModule = createShaderModule(tesselationEvalShaderCode);

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

    /*
    VkPipelineShaderStageCreateInfo tesselationControlShaderStageInfo{};
    tesselationControlShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    tesselationControlShaderStageInfo.stage = VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
    tesselationControlShaderStageInfo.module = tesselationControlModule;
    tesselationControlShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo tesselationEvalShaderStageInfo{};
    tesselationEvalShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    tesselationEvalShaderStageInfo.stage = VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
    tesselationEvalShaderStageInfo.module = tesselationEvalModule;
    tesselationEvalShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, tesselationControlShaderStageInfo, tesselationEvalShaderStageInfo, fragShaderStageInfo};
    */

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
    rasterizer.depthClampEnable = VK_TRUE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = VK_CULL_MODE_BACK_BIT; //VK_CULL_MODE_NONE; used to do this cause i am lazy but VK_CULL_MODE_BACK_BIT IS MORE EFFICIENT
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


    if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &postProcessingGraphicsPipelineLayout) != VK_SUCCESS) {
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
    pipelineInfo.layout = postProcessingGraphicsPipelineLayout;
    pipelineInfo.renderPass = renderPass;
    pipelineInfo.subpass = 0;
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
    
    VK_CHECK(vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &postProcessingGraphicsPipeline));

    vkDestroyShaderModule(device, fragShaderModule, nullptr);
    vkDestroyShaderModule(device, vertShaderModule, nullptr);

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
    renderPassInfo.framebuffer = swapChainHandle.framebuffers[imageIndex];
    renderPassInfo.renderArea.offset = { 0, 0 };
    renderPassInfo.renderArea.extent = swapChainHandle.extent;

    std::array<VkClearValue, 2> clearValues{};
    clearValues[0].color = { {0.00f, 0.00f, 0.001f, 1.0f} };
    clearValues[1].depthStencil = { 1.0f, 0 };

    renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
    renderPassInfo.pClearValues = clearValues.data();

    vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);


    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipelines[currentPipeline]);

    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float)swapChainHandle.extent.width;
    viewport.height = (float)swapChainHandle.extent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

    VkRect2D scissor{};
    scissor.offset = { 0, 0 };
    scissor.extent = swapChainHandle.extent;
    vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

    VkDeviceSize offsets[] = { 0 };

    for (size_t i = 0; i < scene.size(); ++i)
    {
        scene[i]->pipelineIndex = currentPipeline;
        if (scene[i] == mCurrentSelectedModel)
        {
            if (2 < graphicsPipelines.size())
            {
                vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipelines[2]);
                scene[i]->pipelineIndex = 2;
            }
            Draw(scene[i], commandBuffer, pipelineLayouts[scene[i]->pipelineIndex], currentFrame);
            vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipelines[currentPipeline]);
            continue;
        }

        Draw(scene[i], commandBuffer, pipelineLayouts[scene[i]->pipelineIndex], currentFrame);
    }
    if (enableimGUI)
    {
        updateImGui(commandBuffer);
    }
    
    vkCmdEndRenderPass(commandBuffer);
    /*
    
    
    VkDescriptorImageInfo imageInfo = {};
    imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL; // Layout of the image
    imageInfo.imageView = swapChainImageViews[imageIndex];

    VkWriteDescriptorSet descriptorWrite = {};
    descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrite.dstSet = postProcessingDescriptorSet;
    descriptorWrite.dstBinding = 0;
    descriptorWrite.dstArrayElement = 0;
    descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
    descriptorWrite.descriptorCount = 1;
    descriptorWrite.pImageInfo = &imageInfo;

    vkUpdateDescriptorSets(device, 1, &descriptorWrite, 0, nullptr);

    VkAttachmentDescription colorAttachment = {};
    colorAttachment.format = swapChainImageFormat; // Format of the images in the swap chain
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD; // Load previous contents of the attachment
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE; // Store contents to be read later
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR; // Initial layout before the render pass
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR; // Final layout after the render pass

    // Define attachment references
    VkAttachmentReference colorAttachmentRef = {};
    colorAttachmentRef.attachment = 0; // Attachment index
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL; // Layout during the render pass

    // Define subpasses
    VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;

    // Define subpass dependency
    VkSubpassDependency dependency = {};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    // Create render pass
    VkRenderPassCreateInfo renderPassCreateInfo = {};
    renderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassCreateInfo.attachmentCount = 1;
    renderPassCreateInfo.pAttachments = &colorAttachment;
    renderPassCreateInfo.subpassCount = 1;
    renderPassCreateInfo.pSubpasses = &subpass;
    renderPassCreateInfo.dependencyCount = 1;
    renderPassCreateInfo.pDependencies = &dependency;

    VkRenderPass postProcessRenderPass;
    if (vkCreateRenderPass(device, &renderPassCreateInfo, nullptr, &postProcessRenderPass) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create render pass!");
    }

    VkImageView attachments[] = { swapChainImageViews[imageIndex]};
    
    // Create framebuffer
    VkFramebufferCreateInfo framebufferInfo = {};
    framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebufferInfo.renderPass = postProcessRenderPass; // The post-processing render pass
    framebufferInfo.attachmentCount = 1; // Number of attachments
    framebufferInfo.pAttachments = attachments; // Array of image views for attachments
    framebufferInfo.width = swapChainExtent.width; // Width of the framebuffer
    framebufferInfo.height = swapChainExtent.height; // Height of the framebuffer
    framebufferInfo.layers = 1; // Number of layers in the framebuffer

    VkFramebuffer postProcessFramebuffer;
    if (vkCreateFramebuffer(device, &framebufferInfo, nullptr, &postProcessFramebuffer) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create framebuffer!");
    }

    
    // Perform post-processing
    // Begin render pass for post-processing
    VkRenderPassBeginInfo postProcessRenderPassInfo{};
    postProcessRenderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    postProcessRenderPassInfo.renderPass = postProcessRenderPass; // Your post-processing render pass
    postProcessRenderPassInfo.framebuffer = postProcessFramebuffer; // Your post-processing framebuffer
    postProcessRenderPassInfo.renderArea.offset = { 0, 0 };
    postProcessRenderPassInfo.renderArea.extent = swapChainExtent;
    

    std::array<VkClearValue, 1> postProcessClearValues{};
    postProcessClearValues[0].color = { {0.0f, 0.0f, 0.0f, 1.0f} }; // Clear color for post-processing

    postProcessRenderPassInfo.clearValueCount = static_cast<uint32_t>(postProcessClearValues.size());
    postProcessRenderPassInfo.pClearValues = postProcessClearValues.data();

    vkCmdBeginRenderPass(commandBuffer, &postProcessRenderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    // Bind the post-processing graphics pipeline
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, postProcessingGraphicsPipeline); // Your post-processing pipeline

    // Draw a full-screen quad for post-processing
    vkCmdDraw(commandBuffer, 6, 1, 0, 0); // Adjust vertices count as needed
    // 
    
    vkCmdEndRenderPass(commandBuffer);
    */

    // End recording the command buffer
    if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
        throw std::runtime_error("ERROR: failed to record command buffer!");
    }

}

void Engine::Draw(Model* cModel, VkCommandBuffer commandBuffer, VkPipelineLayout graphicsPipelineLayout, int currentFrame)
{
    VkDeviceSize offsets[] = { 0 };

    vkCmdBindVertexBuffers(commandBuffer, 0, 1, &cModel->vertexBuffer, offsets);

    vkCmdBindIndexBuffer(commandBuffer, cModel->indexBuffer, 0, VK_INDEX_TYPE_UINT32);

    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipelineLayout, 0, 1, &cModel->descriptorSets[currentFrame], 0, nullptr);

    vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(cModel->indices.size()), 1, 0, 0, 0);
}



void Engine::mouse_callback(GLFWwindow* window, int button, int action, int mods)
{
    Engine* engine = retrieveEnginePtr(window); //This is so fucking retarded
    // Camera* camera = reinterpret_cast<Engine*>(glfwGetWindowUserPointer(window))->camera; // this is even more disgusting
    if (action != GLFW_PRESS)
        return;
    switch (button)
    {
        case GLFW_MOUSE_BUTTON_2:
        {
            // Basically select a model by click if it is already selected deselect it.
            int index = engine->camera->pickModel(engine->scene, window); 
            if (index != -1 && engine->camera->LockCamera)
            {
                if (engine->scene[index] == engine->mCurrentSelectedModel && engine->mCurrentSelectedModel != nullptr)
                {
                    engine->mCurrentSelectedModel = nullptr;
                }
                else {
                    engine->mCurrentSelectedModel = engine->scene[index];
                }
            }
        }break;
        
        break;        
        default:
            break;
    }
    
}


void Engine::key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    Engine* engine = retrieveEnginePtr(window);

    if (ImGui::GetIO().WantCaptureKeyboard)
    {
        return;
    }

    if (action == GLFW_RELEASE)
    {
        switch (key)
        {
            case GLFW_KEY_K:
            {
                engine->camera->firstClick = false;
            }break;
        default:
            break;
        }
    }

    if (action == GLFW_PRESS)
    {
        switch (key)
        {
            case GLFW_KEY_O: // The physics development is put on hold for now will get back to it once I am mentally ok since physx makes everything 10 times worse
            {
                //physx::PxMat44 mat = glmMat4ToPhysxMat4(engine->mCurrentSelectedModel->transform);
                //engine->physicsEngine->addPhysicsObj(engine->mCurrentSelectedModel, true, mat);
                
            }break;
            case GLFW_KEY_J:
            {
                //engine->physicsEngine->applyForceToRigidBody(engine->scene[3]);
            }break;
            case GLFW_KEY_I:
            {
                //engine->physicsEngine->addColision(engine->scene[3], engine->scene[3]->transform, false);

            }break;
            case GLFW_KEY_N:
            {
              
            }break;
            case GLFW_KEY_P:
            {
            
                engine->currentPipeline = (engine->currentPipeline + 1) % engine->graphicsPipelines.size();
                printf("\r%d", engine->currentPipeline);

            }break;
            case GLFW_KEY_ESCAPE:
            {
                glfwSetWindowShouldClose(window, true);
            }break;
            case GLFW_KEY_K:
            {
                engine->enableimGUI = !engine->enableimGUI;
                if (engine->enableimGUI)
                {
                    engine->io.ConfigFlags &= ~ImGuiConfigFlags_NoMouseCursorChange;
                }
                else {
                    engine->io.ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange;
                }
            
                engine->camera->LockCamera = !engine->camera->LockCamera;
                engine->camera->firstClick = true;
        
            }break;
            case GLFW_KEY_T:
            {
                engine->mCurrentGizmoOperation = ImGuizmo::TRANSLATE;
            }break;
            case GLFW_KEY_R:
            {
                engine->mCurrentGizmoOperation = ImGuizmo::ROTATE;
            }break;
            case GLFW_KEY_Y:
            {
                engine->mCurrentGizmoOperation = ImGuizmo::SCALE;
            }break;
            case GLFW_KEY_U:
            {
                engine->mCurrentGizmoOperation = ImGuizmo::UNIVERSAL;
            }break;
        
            default:
                break;
            }
    }
    return;
}



void Engine::custom_key_callback_wrapper(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    Engine* engine = retrieveEnginePtr(window);

    engine->custom_key_callback(window, key, scancode, action, mods);
    return;
}

void Engine::custom_scroll_callback_wrapper(GLFWwindow* window, double xoffset, double yoffset)
{
    Engine* engine = retrieveEnginePtr(window);

    engine->custom_scroll_callback(window, xoffset, yoffset, engine->camera);

    return;
}

void Engine::custom_mouse_position_callback_wrapper(GLFWwindow* window, double xpos, double ypos)
{
    Engine* engine = retrieveEnginePtr(window);

    engine->custom_mouse_position_callback(window, xpos, ypos);
    
    return;
}

void Engine::custom_mouse_button_callback_wrapper(GLFWwindow* window, int button, int action, int mod)
{
    Engine* engine = retrieveEnginePtr(window);
    
    engine->custom_mouse_button_callback(window, button, action, mod);

    return;
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

    processState();

    uint32_t imageIndex;
    VkResult result = vkAcquireNextImageKHR(device, swapChainHandle.swapChain, UINT64_MAX, imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);

    if (result == VK_ERROR_OUT_OF_DATE_KHR)
    {
        recreate_swapchain(&device, &physicalDevice, &swapChainHandle, &colorImageRes, &depthImageRes, &surface, &renderPass,
            msaaSamples, window, camera, VSync);
        return;
    }
    else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        throw std::runtime_error("ERROR: failed to acquire swap chain image!");
    }

    
    if (scene.size())
    {
        for (size_t i = 0; i < scene.size(); i++)
        {
            updateUniformBuffers(currentFrame, scene[i]);
        }
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

    VkSwapchainKHR swapChains[] = { swapChainHandle.swapChain };
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;

    presentInfo.pImageIndices = &imageIndex;

    result = vkQueuePresentKHR(presentQueue, &presentInfo);
    
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || framebufferResized || swapChainConfigChanged)
    {
        framebufferResized = false;
        swapChainConfigChanged = false;
        recreate_swapchain(&device, &physicalDevice, &swapChainHandle, &colorImageRes, &depthImageRes, &surface, &renderPass,
            msaaSamples, window, camera, VSync);
    } 
    else if (result  != VK_SUCCESS) {
        throw std::runtime_error("ERROR: failed to present swap chain image!");
    } 

    currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;

}

void Engine::processState()
{
    switch (state)
    {
    case STATE_DESTROY_OBJECT:
    {
        if (!scene.size())
        {
            printf("INFO: There is nothing to destroy!\n");
            state = STATE_NOP;
            break;
        }
        
        vkQueueWaitIdle(graphicsQueue);
        cleanUpModel(mCurrentSelectedModel);
        mCurrentSelectedModel = scene.back();
        state = STATE_NOP;
    }break;
    case STATE_RESET_SCENE:
    {
        if (scene.size())
        {

            vkQueueWaitIdle(graphicsQueue);
            for (auto& cModel : scene)
            {
                cleanUpModel(cModel);
                statsFaces -= cModel->statsFaces;
            }
            mCurrentSelectedModel = nullptr;
            scene.resize(0);
            shader_indices.resize(0);
            shader_paths.resize(0);
            
            shader_paths.push_back("res/shaders/shader_vert.spv");
            shader_paths.push_back("res/shaders/shader_frag.spv");
            shader_indices.push_back(std::vector<int>({ 0 , 1 }));
            
        }
        state = STATE_UPDATE_PIPELINE;
    }break;
    case STATE_RESET_AND_UPDATE_SCENE:
    {
        vkQueueWaitIdle(graphicsQueue);
        for (auto& cModel : scene)
        {
            cleanUpModel(cModel);
            statsFaces -= cModel->statsFaces;
        }
        mCurrentSelectedModel = nullptr;
        scene.resize(0);
        shader_indices.resize(0);
        shader_paths.resize(0);

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
        state = STATE_UPDATE_PIPELINE;
    }break;
    case STATE_UPDATE_PIPELINE:
    {
        vkQueueWaitIdle(graphicsQueue);
        for (auto& pipeline : graphicsPipelines)
        {
            vkDestroyPipeline(device, pipeline, nullptr);
        }
        for (auto& pipelineLayout : pipelineLayouts)
        {
            vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
        }
        graphicsPipelines.resize(0); // reset sizes
        pipelineLayouts.resize(0);
        loadShaders();
        state = STATE_NOP;
    }break;
    case STATE_NOP: break;
    default: break;
    }
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
    



void Engine::createDescriptorSetLayoutForModel(Model* model)
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

    if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &model->descriptorSetLayout) != VK_SUCCESS)
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
    static auto startTime = std::chrono::high_resolution_clock::now();

    auto currentTime = std::chrono::high_resolution_clock::now();
    float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

    camera->UpdateMatrices(model);

    UniformBufferObject ubo{};
    ubo.transform = model->transform;
    ubo.view = camera->view;
    ubo.proj = camera->proj;
    ubo.cameraPos = camera->Position;
    ubo.lightPos = camera->lightPos;
    ubo.lightColor = camera->lightColor;
    ubo.material = model->material;
    ubo.time = time;
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

void Engine::createPostProcessingDescriptorSets()
{
    VkDescriptorPoolSize poolSize = {};
    poolSize.type = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE; // Type of descriptor
    poolSize.descriptorCount = 1; // Number of descriptors of this type

    // Create descriptor pool
    VkDescriptorPoolCreateInfo poolInfo = {};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = 1;
    poolInfo.pPoolSizes = &poolSize;
    poolInfo.maxSets = 1; // Maximum number of descriptor sets

    
    if (vkCreateDescriptorPool(device, &poolInfo, nullptr, &postProcessingDescriptionPool) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create descriptor pool!");
    }

    // Define descriptor set layout bindings
    VkDescriptorSetLayoutBinding layoutBinding = {};
    layoutBinding.binding = 0; // Binding point in the shader
    layoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE; // Type of resource
    layoutBinding.descriptorCount = 1; // Number of descriptors in the binding
    layoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT; // Shader stages that can access this binding
    layoutBinding.pImmutableSamplers = nullptr; // Optional

    VkDescriptorSetLayoutCreateInfo layoutInfo = {};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = 1;
    layoutInfo.pBindings = &layoutBinding;

    
    if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &postProcessingDescriptorSetLayout) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create descriptor set layout!");
    }

    // Allocate descriptor sets
    VkDescriptorSetAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = postProcessingDescriptionPool;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &postProcessingDescriptorSetLayout;

    if (vkAllocateDescriptorSets(device, &allocInfo, &postProcessingDescriptorSet) != VK_SUCCESS) {
        throw std::runtime_error("Failed to allocate descriptor sets!");
    }

    // Update descriptor sets
    VkDescriptorImageInfo imageInfo = {};
    imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL; // Layout of the image
    imageInfo.imageView = swapChainHandle.imageViews[0];

    VkWriteDescriptorSet descriptorWrite = {};
    descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrite.dstSet = postProcessingDescriptorSet;
    descriptorWrite.dstBinding = 0;
    descriptorWrite.dstArrayElement = 0;
    descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
    descriptorWrite.descriptorCount = 1;
    descriptorWrite.pImageInfo = &imageInfo;

    vkUpdateDescriptorSets(device, 1, &descriptorWrite, 0, nullptr);
}



void Engine::createTextureImage(Model* model)
{
    int texWidth, texHeight, texChannels;
    stbi_uc* pixels = stbi_load((model->baseDir + model->TEXTURE_PATH).c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
    VkDeviceSize imageSize = texWidth * texHeight * 4; // 4 channels
    model->mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(texWidth, texHeight)))) + 1;

    if (!pixels)
    {
        throw std::runtime_error("ERROR: failed to load texture image!");
    }

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;

    createBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);
    
    void* data;
    vkMapMemory(device, stagingBufferMemory, 0, imageSize, 0, &data);
    memcpy(data, pixels, static_cast<size_t>(imageSize));
    vkUnmapMemory(device, stagingBufferMemory);
    stbi_image_free(pixels);
    
    createImage(&device, &physicalDevice, texWidth, texHeight, model->mipLevels, VK_SAMPLE_COUNT_1_BIT, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, model->textureImage, model->textureImageMemory);
    transitionImageLayout(model->textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, model->mipLevels);
    copyBufferToImage(stagingBuffer, model->textureImage, static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight));
    generateMipmaps(model->textureImage, VK_FORMAT_R8G8B8A8_SRGB, texWidth, texHeight, model->mipLevels);

    vkDestroyBuffer(device, stagingBuffer, nullptr);
    vkFreeMemory(device, stagingBufferMemory, nullptr);
}

void Engine::createNormal(Model* cModel)
{
    
    if (cModel->NORMAL_PATH.empty()) // check if model has a normal texture assigned to it
        return;

    // Create Normal Image
    int norWidth, norHeight, norChannels;
    stbi_uc* pixels = stbi_load((cModel->baseDir + cModel->NORMAL_PATH).c_str(), &norWidth, &norHeight, &norChannels, STBI_rgb_alpha);
    VkDeviceSize imageSize = norWidth * norHeight * 4; // 4 channels
    
    if (!pixels)
    {
        throw std::runtime_error("ERROR: failed to load normal image!");
    }

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;

    createBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

    void* data;
    vkMapMemory(device, stagingBufferMemory, 0, imageSize, 0, &data);
    memcpy(data, pixels, static_cast<size_t>(imageSize));
    vkUnmapMemory(device, stagingBufferMemory);
    stbi_image_free(pixels);

    createImage(&device, &physicalDevice, norWidth, norHeight, cModel->mipLevels, VK_SAMPLE_COUNT_1_BIT, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, cModel->normalImage, cModel->normalImageMemory);
    transitionImageLayout(cModel->normalImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, cModel->mipLevels);
    copyBufferToImage(stagingBuffer, cModel->normalImage, static_cast<uint32_t>(norWidth), static_cast<uint32_t>(norHeight));
    generateMipmaps(cModel->normalImage, VK_FORMAT_R8G8B8A8_SRGB, norWidth, norHeight, cModel->mipLevels);

    vkDestroyBuffer(device, stagingBuffer, nullptr);
    vkFreeMemory(device, stagingBufferMemory, nullptr);

    // Create Normal Image View
    cModel->normalImageView = createImageView(device ,cModel->normalImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT, cModel->mipLevels);

    // Create Normal Image Sampler
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

    if (vkCreateSampler(device, &samplerInfo, nullptr, &cModel->normalSampler) != VK_SUCCESS)
    {
        throw std::runtime_error("ERROR: failed to create normal sampler!");
    }
}



void Engine::generateMipmaps(VkImage image, VkFormat imageFormat,int32_t texWidth, int32_t texHeight, uint32_t mipLevels)
{
    VkFormatProperties formatProperties;
    vkGetPhysicalDeviceFormatProperties(physicalDevice, imageFormat, &formatProperties);

    if (!(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT)) {
        throw std::runtime_error("texture image format does not support linear blitting!");
    }

    VkCommandBuffer commandBuffer = beginSingleTimeCommands(&device, commandPool);

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

    endSingleTimeCommands(&device, commandBuffer, commandPool, graphicsQueue);
}



void Engine::transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t mipLevels) 
{
    VkCommandBuffer commandBuffer = beginSingleTimeCommands(&device, commandPool);

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

    endSingleTimeCommands(&device, commandBuffer, commandPool, graphicsQueue);
}

void Engine::copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height) 
{
    VkCommandBuffer commandBuffer = beginSingleTimeCommands(&device, commandPool);

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

    endSingleTimeCommands(&device, commandBuffer, commandPool, graphicsQueue);
}

void Engine::createTextureImageView(Model* model)
{
    model->textureImageView = createImageView(device ,model->textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT, model->mipLevels);

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


void Engine::loadFile(std::string filename)
{
    
    std::ifstream f{"res/data/user/" + filename};
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
    
    shader_paths = j["Section4"]["ShaderPaths"];
    shader_indices = j["Section4"]["ShaderIndices"];
}

void Engine::loadShaders()
{
    for (int i = 0; i < shader_indices.size(); i++)
    {
        createGraphicsPipelineWrapper(shader_paths[shader_indices[i][0]], shader_paths[shader_indices[i][1]]);
    }
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
    std::ofstream f("res/data/user/" + scene_path);
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
    
    for (auto& shader_path : shader_paths)
    {
        j["Section4"]["ShaderPaths"].push_back(shader_path);
    }
    for (auto& shader_index : shader_indices)
    {
        j["Section4"]["ShaderIndices"].push_back(shader_index);
    }

    f << std::setw(4) << j << std::endl;
}

void Engine::deleteFile(std::string fileName)
{
    std::string finalString = "res/data/user/" + fileName;
    if (std::remove(finalString.c_str()) == 0)
    {
        printf("Succesfully deleted file: %s", fileName.c_str());
    }
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
#ifndef ENGINE_DISABLE_LOGGING
    auto progress = tlog::progress(N);
#endif
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
#ifndef ENGINE_DISABLE_LOGGING
        prog++;
        std::this_thread::sleep_for(std::chrono::milliseconds{100});
        progress.update(prog);
#endif
    }

    model->material.ambient = glm::vec3(materials[0].ambient[0], materials[0].ambient[1], materials[0].ambient[2]);
    model->material.diffuse = glm::vec3(materials[0].diffuse[0], materials[0].diffuse[1], materials[0].diffuse[2]);
    model->material.specular = glm::vec3(materials[0].specular[0], materials[0].specular[1], materials[0].specular[2]);
    model->material.shininess = glm::vec3(materials[0].shininess, 0.0f, 0.0f);
    
    model->statsFaces = static_cast<int>((model->indices.size()/3));
#ifndef ENGINE_DISABLE_LOGGING
    printf("\n");
    tlog::none();
    tlog::success();
    printf("Material count: %d \n", static_cast<int>(materials.size()));
#endif
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

            model_paths.push_back(util::clear_slash(outfilename_str));
        }
    }
    for (const auto& entry : std::filesystem::directory_iterator(textureDirectory))
    {
        std::filesystem::path outfilename = entry.path();
        if (outfilename.extension() == ".png" || outfilename.extension() == ".jpg" || outfilename.extension() == "jpeg")
        {
            std::string outfilename_str = outfilename.string();
    
            texture_paths.push_back(util::clear_slash(outfilename_str));
        }
    }
}

void Engine::findFiles(std::string sceneDirectory, std::string fileExtension)
{
    
    scene_paths.resize(0);
    for (const auto& entry : std::filesystem::directory_iterator(sceneDirectory))
    {
        std::filesystem::path outfilename = entry.path();
        if (outfilename.extension() == fileExtension)
        {
            std::string outfilename_str = outfilename.string();
            scene_paths.push_back(util::clear_slash(outfilename_str));
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
        fps = double(nbFrames) / delta;
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
    std::ifstream f{"res/data/user/" + scene_path};
    
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

void Engine::resetScene(bool toUpdate)
{
    state = (toUpdate) ? STATE_RESET_AND_UPDATE_SCENE : STATE_RESET_SCENE;
}

void Engine::addtoScene(Model* model)
{
    
    scene.push_back(model);
}

void Engine::setCustomCameraFunction(std::function<void(GLFWwindow*,Camera*)> customCameraFunction)
{
    this->customCameraFunction = customCameraFunction;
}

void Engine::setCustomMainUpdate(std::function<void(void)> custom_main_update)
{
    this->custom_main_update = custom_main_update;        
}


void Engine::setCustomKeyCallbackFunction(std::function<void(GLFWwindow*, int, int, int, int)> custom_key_callback)
{
    this->custom_key_callback = custom_key_callback;
}

void Engine::setCustomScrollCallbackFunction(std::function<void(GLFWwindow*, double, double, Camera* camera)> custom_scroll_callback)
{
    this->custom_scroll_callback = custom_scroll_callback;
}

void Engine::setCustomMousePositionCallbackFunction(std::function<void(GLFWwindow*, double, double)> custom_mouse_position_callback)
{
    this->custom_mouse_position_callback = custom_mouse_position_callback;
}

void Engine::setCustomMouseButtonCallbackFunction(std::function<void(GLFWwindow*, int, int, int)> custom_mouse_button_callback)
{
    this->custom_mouse_button_callback = custom_mouse_button_callback;
}

void Engine::cleanUpModel(Model* model)
{
    
    for (auto& ubo : model->uniformBuffers)
        vkDestroyBuffer(device, ubo, nullptr);
    for (auto& mem : model->uniformBuffersMemory)
        vkFreeMemory(device, mem, nullptr);
    vkDestroyDescriptorPool(device, model->descriptorPool, nullptr);
    if (!model->NORMAL_PATH.empty())
    {
        vkDestroySampler(device, model->normalSampler, nullptr);
        vkDestroyImageView(device, model->normalImageView, nullptr);
        vkDestroyImage(device, model->normalImage, nullptr);
        vkFreeMemory(device, model->normalImageMemory, nullptr);
        vkDestroyDescriptorSetLayout(device, model->descriptorSetLayout, nullptr);
    }

    vkDestroySampler(device, model->textureSampler, nullptr);
    vkDestroyImageView(device, model->textureImageView, nullptr);
    vkDestroyImage(device, model->textureImage, nullptr);
    vkFreeMemory(device, model->textureImageMemory, nullptr);
    vkDestroyBuffer(device, model->vertexBuffer, nullptr);

    vkFreeMemory(device, model->vertexBufferMemory, nullptr);
    vkDestroyBuffer(device, model->indexBuffer, nullptr);
    vkFreeMemory(device, model->indexBufferMemory, nullptr);
    free(model);

}


void Engine::fmod_update(GLFWwindow* window)
{
    
    static Engine* engine = retrieveEnginePtr(window);
    if (engine->enableAudio)
    {
        while (!glfwWindowShouldClose(window))
        {
            engine->audioMgr->Update();
        }
    }
    
}

void Engine::cleanup()
{
    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

//    physicsEngine->~PhysicsEngine();
//    delete physicsEngine;

    audioMgr->~Audio();
    delete audioMgr;
    
    delete camera;

    cleanupSwapChain(&device, &swapChainHandle, &colorImageRes, &depthImageRes);

    vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);
    vkDestroyDescriptorSetLayout(device, postProcessingDescriptorSetLayout, nullptr);

    vkDestroyDescriptorPool(device, imGuiDP, nullptr);
    vkDestroyDescriptorPool(device, postProcessingDescriptionPool, nullptr);
    

    for (size_t i = 0; i < scene.size(); ++i)
    {
        
        cleanUpModel(scene[i]);
    }

    vkDestroyPipeline(device, postProcessingGraphicsPipeline, nullptr);
    vkDestroyPipelineLayout(device, postProcessingGraphicsPipelineLayout, nullptr);
    for (auto& pipeline : graphicsPipelines)
    {
        vkDestroyPipeline(device, pipeline, nullptr);
    }
    for (auto& pipelineLayout : pipelineLayouts)
    {
        vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
    }
    vkDestroyRenderPass(device, renderPass, nullptr);

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) { // destroy sync Objects
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

