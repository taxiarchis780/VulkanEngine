#ifndef __GUI_SRC__
#define __GUI_SRC__
#include "Engine.h"



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


void Engine::initImGui()
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    io = ImGui::GetIO(); (void)io;
    io.Fonts->AddFontFromFileTTF("res/fonts/OpenSans-Regular.ttf", 18.0f);
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange;   // Disable the fucking mouse
    io.IniFilename = "res/config/imgui.ini";
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
    VkCommandBuffer commandBuffer = beginSingleTimeCommands(&device, commandPool);
    ImGui_ImplVulkan_CreateFontsTexture(commandBuffer);
    endSingleTimeCommands(&device, commandBuffer, commandPool, graphicsQueue);
    vkDeviceWaitIdle(device);

    ImGui_ImplVulkan_DestroyFontUploadObjects();

    mCurrentGizmoOperation = ImGuizmo::TRANSLATE;
    mCurrentGizmoMode = ImGuizmo::LOCAL;
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
    
    static bool isComponentSelected = true;
    ImGui::NewFrame();

    //ImGui::SetNextWindowPos(ImVec2(swapChainExtent.width - 735 , 0));
    if (isComponentSelected) ImGui::SetNextWindowSize(ImVec2(427, swapChainHandle.extent.height));
    else ImGui::SetNextWindowSize(ImVec2(600, swapChainHandle.extent.height));
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::Begin("Main", 0, ImGuiWindowFlags_NoMove);
    ImGui::BeginTabBar("##TabBar");
    if (ImGui::BeginTabItem("Components"))
    {
        isComponentSelected = true;
        ImGui::Text("Renderer: %s", RendererName.c_str());

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




        ImGui::Checkbox("EditScene", &editScene);
        if (editScene)
        {
            ImGui::InputText("SceneName", &scene_name);
            if (ImGui::Button("Refresh"))
            {
                findFiles("res/data/user/", ".json");
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
            if (ImGui::Button("LoadScene"))
            {
                resetScene(ENGINE_LOAD_SCENE);
            } ImGui::SameLine();
            if (ImGui::Button("Delete Scene"))
            {
                
                deleteFile(scene_path);
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
            if (ImGui::Button("New Scene"))
            {
                scene_path = "res/data/system/empty.json";
                resetScene(ENGINE_RESET_SCENE);
            }
        }

        if (ImGui::Button("Update Graphics Pipeline"))
        {
           state = STATE_UPDATE_PIPELINE;
        }


        ImGui::InputText("VertexPath", &vert_path);
        ImGui::InputText("FragmentPath", &frag_path);

        if (ImGui::Button("Create Graphics Pipeline"))
        {
            createGraphicsPipelineWrapper(vert_path, frag_path);
            shader_paths.push_back(vert_path);
            shader_paths.push_back(frag_path);

            shader_indices.push_back(std::vector<int>({ static_cast<int>(shader_paths.size() - 1), static_cast<int>(shader_paths.size()) }));

        }
        ImGui::SliderFloat3("LightPos", glm::value_ptr(camera->lightPos), -5.0f, 5.0f, NULL);
        ImGui::ColorPicker3("lightColor", glm::value_ptr(camera->lightColor), ImGuiColorEditFlags_DisplayRGB | ImGuiColorEditFlags_NoAlpha);

        if (ImGui::SliderFloat("AudioVolume", &audioMgr->volume, 0.0f, 1.0f, NULL))
        {
            audioMgr->changeVolume();
        }
        if (mCurrentSelectedModel)
        {
            ImGui::InputText("UUID", &mCurrentSelectedModel->UUID);
            if (ImGui::Button("Generate UUID"))
            {
                util::GenerateUUID(mCurrentSelectedModel, 8, true);
            }
            ImGui::SliderFloat("FOV", &camera->FOV, 10.0f, 120.0f, NULL);
            ImGui::SliderFloat3("ModelPos", glm::value_ptr(mCurrentSelectedModel->translationVec), -15.0f, 15.0f, NULL);
            ImGui::SliderFloat3("ModelRot", glm::value_ptr(mCurrentSelectedModel->rotationVec), 0.0f, 6.28f, NULL);
            ImGui::SliderFloat3("ModelScale", glm::value_ptr(mCurrentSelectedModel->scaleVec), 0.001f, 10.0f, NULL);
            



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
            ImGui::Text("GuizmoMode");
            if (ImGui::RadioButton("Local", mCurrentGizmoMode == ImGuizmo::LOCAL))
                mCurrentGizmoMode = ImGuizmo::LOCAL;
            ImGui::SameLine();
            if (ImGui::RadioButton("World", mCurrentGizmoMode == ImGuizmo::WORLD))
                mCurrentGizmoMode = ImGuizmo::WORLD;


            //ImGuiColorEditFlags_PickerHueWheel
            ImGui::Text("Selected Model Material");
            ImGui::SliderFloat3("Ambient", glm::value_ptr(mCurrentSelectedModel->material.ambient), 0.0f, 10.0f, NULL);
            ImGui::SliderFloat3("Diffuse", glm::value_ptr(mCurrentSelectedModel->material.diffuse), 0.0f, 10.0f, NULL);
            ImGui::SliderFloat3("Specular", glm::value_ptr(mCurrentSelectedModel->material.specular), 0.0f, 10.0f, NULL);
            ImGui::SliderFloat("Shininess", &mCurrentSelectedModel->material.shininess.r, 0.1f, 360.0f, NULL, ImGuiSliderFlags_Logarithmic);

        }

        ImGui::InputText("Model", &model_path);
        ImGui::InputText("Texture", &texture_path);
        if (ImGui::Button("Add Model"))
        {
            Model* cModel = nullptr;
            try {
                cModel = new Model("models/" + model_path, "textures/" + texture_path);
                util::GenerateUUID(cModel, 8, true);
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
            catch (const std::exception& e) {
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
            state = STATE_DESTROY_OBJECT;

            scene.resize(0);
            for (size_t i = 0; i < newScene.size(); i++)
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

        ImGui::EndTabItem();
    }

    if (ImGui::BeginTabItem("Scene"))
    {
        isComponentSelected = false;
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
        ImGui::EndTabItem();
        
    }
    if (ImGui::BeginTabItem("Settings"))
    {
        
        if (ImGui::Checkbox("Change Present Mode", &PresentModeChange) || PresentModeChange)
        {
            if(ImGui::BeginListBox("MODES", ImVec2(200, 160)))
            {
                
                for (size_t i = 0; i < 4; ++i)
                {
                    const bool isSelected = (swapChainHandle.presentMode == i);
                    if (ImGui::Selectable(presentModes[i], isSelected))
                    {
                        swapChainHandle.presentMode = (VkPresentModeKHR)i;
                        swapChainConfigChanged = true;
                        
                    }
                    if (isSelected)
                        ImGui::SetItemDefaultFocus();
                }
                ImGui::EndListBox();
            }
        }
        
        ImGui::EndTabItem();
    }
    
    ImGui::EndTabBar();
    ImGui::End();

    
    {
        ImGuizmo::SetOrthographic(false);
        ImGuizmo::SetDrawlist(ImGui::GetForegroundDrawList());

        ImGuizmo::SetRect(0, 0, swapChainHandle.extent.width, swapChainHandle.extent.height);
        if (!lightTranslateEnable && mCurrentSelectedModel != nullptr)
        {
            ImGuizmo::Manipulate(glm::value_ptr(camera->view), glm::value_ptr(camera->proj), mCurrentGizmoOperation, mCurrentGizmoMode, glm::value_ptr(mCurrentSelectedModel->transform));

            if (ImGuizmo::IsUsing())
            {
                glm::vec3 translation, rotation, scale;
                util::DecomposeTransform(mCurrentSelectedModel->transform, translation, rotation, scale);
                mCurrentSelectedModel->translationVec = translation;
                mCurrentSelectedModel->rotationVec = rotation;
                mCurrentSelectedModel->scaleVec = scale;
            }
        }
        else if (mCurrentSelectedModel != nullptr) {
            ImGuizmo::Manipulate(glm::value_ptr(camera->view), glm::value_ptr(camera->proj), mCurrentGizmoOperation, mCurrentGizmoMode, glm::value_ptr(camera->lightMat));

            if (ImGuizmo::IsUsing())
            {
                glm::vec3 translation, rotation, scale;
                util::DecomposeTransform(camera->lightMat, translation, rotation, scale);
                camera->lightPos = translation;

            }
        }

    }

    ImGui::Render();
}




#endif