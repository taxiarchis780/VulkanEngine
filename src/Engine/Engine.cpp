#include "Engine.h"
#include <map>
#include <sstream>

#include <unordered_map>

#define VK_CHECK(x) do {VkResult err = x;if (err){ printf("Detected Vulkan error %d at %s:%d.\n", int(err), __FILE__, __LINE__); abort();}} while (0)

Engine* retrieveEnginePtr(GLFWwindow * window) {
    return reinterpret_cast<Engine*>(glfwGetWindowUserPointer(window));
}

void framebufferResizeCallback(GLFWwindow * window, int width, int height)
{
    auto engine = reinterpret_cast<Engine*>(glfwGetWindowUserPointer(window));
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

    tlog::info("Initialization completed succesfully");

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


HWND Engine::getHWND()
{
    return this->hwnd;
}

void Engine::initWindow()
{
    glfwInit();
    
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

    window = glfwCreateWindow(WIDTH, HEIGHT, TITLE, nullptr, nullptr);
    monitor = glfwGetPrimaryMonitor();
    videoMode = glfwGetVideoMode(monitor);
    hwnd = glfwGetWin32Window(window);
    glfwSetInputMode(window, GLFW_STICKY_MOUSE_BUTTONS, GLFW_TRUE);
    glfwSetInputMode(window, GLFW_STICKY_KEYS, GLFW_TRUE);
    if (!glfwRawMouseMotionSupported())
        glfwSetInputMode(window, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);

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
        glfwSetMouseButtonCallback(window, mouse_callback);
    }
    glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);

}

void thread_function(VkDevice* device, VkPhysicalDevice* physicalDevice, VkSurfaceKHR* surface, VkQueue graphicsQueue, std::vector<Model*>* models, it_ImageResource* depthRes ,std::vector<VkBuffer>& lightBuffers, std::mutex& queueMutex) {


    // Create a command pool for this thread
    VkCommandPool commandPool;
    
    create_command_pool(device, physicalDevice, &commandPool, surface);

    for (Model* model : *models) {

        mt_init_model_resources(device, physicalDevice, commandPool, graphicsQueue, model, depthRes, &lightBuffers, queueMutex);
    }

    vkDestroyCommandPool(*device, commandPool, nullptr);
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
    create_shadow_render_pass(&device, &physicalDevice, &shadowRenderPass, msaaSamples);
    create_descriptor_set_layout(&device, &descriptorSetLayout);
    

    create_command_pool(&device, &physicalDevice, &commandPool, &surface);
    
    create_color_resources(&device, &physicalDevice, &colorImageRes, swapChainHandle.imageFormat, swapChainHandle.extent, msaaSamples);
    create_depth_resources(&device, &physicalDevice, &depthImageRes, swapChainHandle.imageFormat, swapChainHandle.extent, msaaSamples);
    create_shadow_resources(&device, &physicalDevice, &shadowImageRes, swapChainHandle.imageFormat, swapChainHandle.extent, msaaSamples);
    create_framebuffers(&device, &swapChainHandle.framebuffers, swapChainHandle.imageViews, swapChainHandle.extent, renderPass, colorImageRes.imageView, depthImageRes.imageView);
    create_shadow_framebuffer(&device, &shadowFramebuffer, &shadowImageRes.imageView, &shadowRenderPass, swapChainHandle.extent);
    
    create_light_uniform_buffer(&device, &physicalDevice, &lightRes);

    if (firstScene)
        scene_path = "main.json";
    camera = new Camera(swapChainHandle.extent.width, swapChainHandle.extent.height);
    //loadScene();
    //clock_t t = clock();
    load_scene(scene_path, &scene, &sceneSize);
    

    load_file(scene_path, &shader_paths, &shader_indices, &scene, sceneSize, camera);

    loadShaders();

    create_shadow_pipeline(&device, "res/shaders/shadow_vert.spv", "res/shaders/shadow_frag.spv", &shadowPipelineLayout, &shadowPipeline, descriptorSetLayout, shadowRenderPass, swapChainHandle.extent, msaaSamples);

 

    std::vector<std::thread> threads;
    std::mutex queueMutex;

    auto start = std::chrono::high_resolution_clock::now();
    
    // Number of threads to use
    unsigned int num_threads = std::thread::hardware_concurrency();
    printf("\n%d\n", num_threads);
    if (num_threads == 0) num_threads = 1; // Fallback in case hardware_concurrency() returns 0

    // Split work among threads
    std::vector<std::vector<Model*>> model_batches(num_threads);
    for (size_t i = 0; i < sceneSize; ++i) {
        model_batches[i % num_threads].push_back(scene[i]);
    }

    for (unsigned int i = 0; i < num_threads; ++i) {
        if (!model_batches[i].empty()) {
            threads.emplace_back(thread_function, &device, &physicalDevice, &surface, graphicsQueue, &model_batches[i], &shadowImageRes, std::ref(lightRes.lightBuffers), std::ref(queueMutex));
            //void thread_function(VkDevice * device, VkPhysicalDevice * physicalDevice, 
            // VkSurfaceKHR * surface, VkQueue graphicsQueue, std::vector<Model*>*models, std::vector<VkBuffer>&lightBuffers)
        }
    }

    // Join remaining threads
    for (auto& th : threads) {
        if (th.joinable()) {
            th.join();
        }
    }
    //for (size_t i = 0; i < sceneSize; ++i)
    //{
    //    init_model_resources(&device, &physicalDevice, commandPool, graphicsQueue, scene[i], &lightRes.lightBuffers);
    //}
    
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> duration = end - start;
    std::cout << "Time taken for multithread: " << duration.count() << " seconds" << std::endl;



    createImGuiDP();
    
    mCurrentSelectedModel = nullptr;

    create_commandbuffer(&device, &commandBuffers, commandPool);
    create_sync_objects(&device, &imageAvailableSemaphores, &renderFinishedSemaphores, &inFlightFences);

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
        double deltaTime = currentTime - lastTime;
        
        //std::this_thread::sleep_for(std::chrono::milliseconds(16));
        if (deltaTime >= (1.0/videoMode->refreshRate))
        {   
            camera->UpdateInputs(window, customCameraFunction);
            //physicsEngine->Update(); // project on hold
            
            lastTime = currentTime;
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


void Engine::recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex)
{
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
        throw std::runtime_error("failed to begin recording command buffer!");
    }

    VkClearValue clearValue = {};
    clearValue.depthStencil = { 1.0f, 0 };
    


    VkRenderPassBeginInfo shadowRenderPassInfo = {};
    shadowRenderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    shadowRenderPassInfo.renderPass = shadowRenderPass;
    shadowRenderPassInfo.framebuffer = shadowFramebuffer;
    shadowRenderPassInfo.renderArea.offset = { 0, 0 };
    shadowRenderPassInfo.renderArea.extent = swapChainHandle.extent;
    shadowRenderPassInfo.clearValueCount = 1;
    shadowRenderPassInfo.pClearValues = &clearValue;

    transition_image_layout(&device, commandPool, graphicsQueue, shadowImageRes.image, VK_FORMAT_D32_SFLOAT,
        VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, 1);

    vkCmdBeginRenderPass(commandBuffer, &shadowRenderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, shadowPipeline);

    // Bind vertex buffer, set viewport, scissor, etc.
    // Render scene from the light's perspective to create shadow map
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
        if (scene[i]->UUID == "skybox") continue;
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, &scene[i]->vertexBuffer, offsets);

        vkCmdBindIndexBuffer(commandBuffer, scene[i]->indexBuffer, 0, VK_INDEX_TYPE_UINT32);

        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, shadowPipelineLayout, 0, 1, &scene[i]->descriptorSets[currentFrame], 0, nullptr);

        vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(scene[i]->indices.size()), 1, 0, 0, 0);
    }
    

    vkCmdEndRenderPass(commandBuffer);


    transition_image_layout(&device, commandPool, graphicsQueue, shadowImageRes.image, VK_FORMAT_D32_SFLOAT,
        VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL, 1);

    


    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = renderPass;
    renderPassInfo.framebuffer = swapChainHandle.framebuffers[imageIndex];
    renderPassInfo.renderArea.offset = { 0, 0 };
    renderPassInfo.renderArea.extent = swapChainHandle.extent;

    std::array<VkClearValue, 2> clearValues{};
    clearValues[0].color = { {0.00f, 0.00f, 0.000f, 1.0f} };
    clearValues[1].depthStencil = { 1.0f, 0 };

    renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
    renderPassInfo.pClearValues = clearValues.data();

    vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);


    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipelines[currentPipeline]);
    

    

    /*
    * CHANGE THE WAY MODELS ARE DRAWN
    * LOOP THROUGH THE AVAILABLE PIPELINES
    * LOOP THROUGH THE SCENE FOR EACH MODEL
    * IF scene[i]->pipelineIndex == currentPipeline
    * DRAW MODEL ELSE CONTINUE
    */
    
    for (size_t currentGPipeline = 0; currentGPipeline < graphicsPipelines.size(); currentGPipeline++)
    {
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipelines[currentGPipeline]);
        for (size_t current_model = 0; current_model < scene.size(); current_model++)
        {
            if (scene[current_model]->pipelineIndex == currentGPipeline)
            {
                draw_model(scene[current_model], commandBuffer, pipelineLayouts[scene[current_model]->pipelineIndex], currentFrame);
            }
        }
    }
    /*
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
            draw_model(scene[i], commandBuffer, pipelineLayouts[scene[i]->pipelineIndex], currentFrame);
            vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipelines[currentPipeline]);
            continue;
        }

        if (scene[i]->UUID == "skybox")
        {
            vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipelines[3]);
            draw_model(scene[i], commandBuffer, pipelineLayouts[scene[i]->pipelineIndex], currentFrame);
            vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipelines[currentPipeline]);
        }

        draw_model(scene[i], commandBuffer, pipelineLayouts[scene[i]->pipelineIndex], currentFrame);
    }*/

    
    if (enableimGUI)
    {
        updateImGui(commandBuffer);
    }
    
    vkCmdEndRenderPass(commandBuffer);
    
    // End recording the command buffer
    if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
        throw std::runtime_error("ERROR: failed to record command buffer!");
    }

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
            
                //engine->currentPipeline = (engine->currentPipeline + 1) % (engine->graphicsPipelines.size() - 1);
                //printf("\r%d", engine->currentPipeline);

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

    

    update_light_uniform_buffers(&lightRes, camera, currentFrame);

    
    if (scene.size())
    {
        for (size_t i = 0; i < scene.size(); i++)
        {
            update_model_uniform_buffers(scene[i], camera, currentFrame);
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
        cleanup_model(&device, mCurrentSelectedModel);
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
                cleanup_model(&device, cModel);

            }
            mCurrentSelectedModel = nullptr;
            scene.resize(0);
            shader_indices.resize(0);
            shader_paths.resize(0);
            
            shader_paths.push_back("res/shaders/shader_vert.spv");
            shader_paths.push_back("res/shaders/shader_no_normal_frag.spv");
            shader_indices.push_back(std::vector<int>({ 0 , 1 }));
            
        }
        state = STATE_UPDATE_PIPELINE;
    }break;
    case STATE_RESET_AND_UPDATE_SCENE:
    {
        vkQueueWaitIdle(graphicsQueue);
        for (auto& cModel : scene)
        {
            cleanup_model(&device, cModel);

        }
        mCurrentSelectedModel = nullptr;
        scene.resize(0);
        shader_indices.resize(0);
        shader_paths.resize(0);

        load_scene(scene_path, &scene, &sceneSize);
        load_file(scene_path, &shader_paths, &shader_indices, &scene, sceneSize, camera);

        for (size_t i = 0; i < sceneSize; ++i)
        {
            init_model_resources(&device, &physicalDevice, commandPool, graphicsQueue, scene[i], &shadowImageRes, &lightRes.lightBuffers);
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


void Engine::loadShaders()
{
    for (int i = 0; i < shader_indices.size(); i++){
        create_graphics_pipeline(&device, static_cast<int>(graphicsPipelines.size()), shader_paths[shader_indices[i][0]], shader_paths[shader_indices[i][1]],
            &pipelineLayouts, &graphicsPipelines, msaaSamples, descriptorSetLayout, renderPass);
    }
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


void Engine::drawWindowTitle()
{
    double currentTime = glfwGetTime();
    double delta = currentTime - lastTimeWindowTitle;
    nbFrames++;
    double fps = 60.0;
    if (delta >= 1.0)
    {
        double timeToDraw = 1000.0 / double(nbFrames);
        fps = double(nbFrames) / delta;
        std::stringstream ss;
        ss << TITLE << " " << VERSION << " [" << fps << " FPS]" << " [" << timeToDraw << "ms  Frametime ]";

        glfwSetWindowTitle(window, ss.str().c_str());

        nbFrames = 0;
        lastTimeWindowTitle = currentTime;
    }
    
}



void Engine::resetScene(bool toUpdate)
{
    state = (toUpdate) ? STATE_RESET_AND_UPDATE_SCENE : STATE_RESET_SCENE;
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

//  physicsEngine->~PhysicsEngine();
//  delete physicsEngine;

    audioMgr->~Audio();
    delete audioMgr;
    
    delete camera;

    cleanupSwapChain(&device, &swapChainHandle, &colorImageRes, &depthImageRes);

    vkDestroyImageView(device, shadowImageRes.imageView, nullptr);
    vkDestroyImage(device, shadowImageRes.image, nullptr);
    vkFreeMemory(device, shadowImageRes.memory, nullptr);
    vkDestroySampler(device, shadowImageRes.sampler, nullptr);

    vkDestroyFramebuffer(device, shadowFramebuffer, nullptr);

    vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);

    vkDestroyDescriptorPool(device, imGuiDP, nullptr);
    
    cleanup_light_uniform_buffer(device, &lightRes);

    for (size_t i = 0; i < scene.size(); ++i)
    {
        
        cleanup_model(&device, scene[i]);
    }

    for (auto& pipeline : graphicsPipelines)
    {
        vkDestroyPipeline(device, pipeline, nullptr);
    }
    vkDestroyPipeline(device, shadowPipeline, nullptr);
    for (auto& pipelineLayout : pipelineLayouts)
    {
        vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
    }
    vkDestroyPipelineLayout(device, shadowPipelineLayout, nullptr);
    vkDestroyRenderPass(device, renderPass, nullptr);
    vkDestroyRenderPass(device, shadowRenderPass, nullptr);

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) { // destroy sync Objects
        vkDestroySemaphore(device, renderFinishedSemaphores[i], nullptr);
        vkDestroySemaphore(device, imageAvailableSemaphores[i], nullptr);
        vkDestroyFence(device, inFlightFences[i], nullptr);
    }

    vkDestroyCommandPool(device, commandPool, nullptr);

    vkDestroyDevice(device, nullptr);

    

    vkDestroySurfaceKHR(instance, surface, nullptr);
    vkDestroyInstance(instance, nullptr);


    glfwDestroyWindow(window);

    glfwTerminate();   
}