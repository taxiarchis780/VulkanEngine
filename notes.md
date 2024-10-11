/*static ImGuiTableFlags flags = ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable | ImGuiTableFlags_NoClip;
    if (ImGui::BeginTable("Vertices2", 4, flags))
    {
        
        ImGui::TableSetupColumn("One");
        ImGui::TableSetupColumn("Two");
        ImGui::TableSetupColumn("Three");
        ImGui::TableSetupColumn("Four");
        ImGui::TableHeadersRow();
        

        for (int row = 0; row < 10000; row++)
        {
            
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::Text("Vert %d", row);
            ImGui::TableNextColumn();
            ImGui::Text("Some contents");
            ImGui::TableNextColumn();
            ImGui::Text("123.123");
            ImGui::TableNextColumn();
            ImGui::Text("idk");
        }
        ImGui::EndTable();
    }
*/


/*
    if (button == GLFW_MOUSE_BUTTON_2 && action == GLFW_PRESS)
    {
        int index = engine->camera->pickModel(engine->scene, window);
        if (index != -1 && engine->camera->LockCamera)
        {
            engine->mCurrentSelectedModel = engine->scene[index];
        }
    }
*/
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







  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
     /$$$$$$$   /$$$$$$  /$$   /$$ /$$ /$$$$$$$$       /$$$$$$$$ /$$$$$$  /$$   /$$  /$$$$$$  /$$   /$$        /$$$$$$  /$$   /$$ /$$     /$$ /$$$$$$$$ /$$   /$$ /$$$$$$ /$$   /$$  /$$$$$$ 
    | $$__  $$ /$$__  $$| $$$ | $$| $/|__  $$__/      |__  $$__//$$__  $$| $$  | $$ /$$__  $$| $$  | $$       /$$__  $$| $$$ | $$|  $$   /$$/|__  $$__/| $$  | $$|_  $$_/| $$$ | $$ /$$__  $$
    | $$  \ $$| $$  \ $$| $$$$| $$|_/    | $$            | $$  | $$  \ $$| $$  | $$| $$  \__/| $$  | $$      | $$  \ $$| $$$$| $$ \  $$ /$$/    | $$   | $$  | $$  | $$  | $$$$| $$| $$  \__/
    | $$  | $$| $$  | $$| $$ $$ $$       | $$            | $$  | $$  | $$| $$  | $$| $$      | $$$$$$$$      | $$$$$$$$| $$ $$ $$  \  $$$$/     | $$   | $$$$$$$$  | $$  | $$ $$ $$| $$ /$$$$
    | $$  | $$| $$  | $$| $$  $$$$       | $$            | $$  | $$  | $$| $$  | $$| $$      | $$__  $$      | $$__  $$| $$  $$$$   \  $$/      | $$   | $$__  $$  | $$  | $$  $$$$| $$|_  $$
    | $$  | $$| $$  | $$| $$\  $$$       | $$            | $$  | $$  | $$| $$  | $$| $$    $$| $$  | $$      | $$  | $$| $$\  $$$    | $$       | $$   | $$  | $$  | $$  | $$\  $$$| $$  \ $$
    | $$$$$$$/|  $$$$$$/| $$ \  $$       | $$            | $$  |  $$$$$$/|  $$$$$$/|  $$$$$$/| $$  | $$      | $$  | $$| $$ \  $$    | $$       | $$   | $$  | $$ /$$$$$$| $$ \  $$|  $$$$$$/
    |_______/  \______/ |__/  \__/       |__/            |__/   \______/  \______/  \______/ |__/  |__/      |__/  |__/|__/  \__/    |__/       |__/   |__/  |__/|______/|__/  \__/ \______/ 
  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~                                                                                                                                                                                       

#version 460 core
#extension GL_EXT_ray_tracing : disable

layout(binding = 1) uniform sampler2D texSampler;
layout(binding = 2) uniform sampler2D normalMap;

struct Material 
{
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
    vec3 shininess;
};

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec3 fragPos;
layout(location = 3) in vec3 aNormal;
layout(location = 4) in vec3 aTangent;
layout(location = 5) in vec3 aBitangent;
layout(location = 6) in vec3 lightPos;
layout(location = 7) in vec3 aCameraPos;
layout(location = 8) in vec3 alightColor;
layout(location = 9) in Material mMaterial;


layout(location = 0) out vec4 outColor;

float LinearizeDepth(float depth) 
{
    float z = depth * 2.0 - 1.0; // back to NDC 
    return (2.0 * 0.1f * 100.0f) / (100.0f + 0.1f - z * (100.0f - 0.1f));	
}



void main() {

    // Fetch the normal from the normal map and transform it to [-1, 1] range
    vec3 normal = normalize(aNormal);
    vec3 tangent = normalize(aTangent);
    vec3 bitangent = normalize(aBitangent);

    mat3 TBN = mat3(tangent, bitangent, normal);
    vec3 worldNormal = (texture(normalMap, fragTexCoord).xyz );
    vec3 perturbedNormal = normalize(TBN * worldNormal);

    // ambient
    float ambientStrength = 0.05f;
    vec3 ambient = mMaterial.ambient * ambientStrength * alightColor;
    
    // diffuse
    vec3 lightDir = normalize(lightPos - fragPos);
    float diff = max(dot(perturbedNormal, lightDir), 0.0f);
    
    vec3 diffuse = (diff * mMaterial.diffuse )* alightColor;
    
    
    
    // specular
    float specularStrength = mMaterial.shininess.r;
    vec3 viewDir = normalize(aCameraPos - fragPos);
    vec3 reflectDir = reflect(-lightDir, perturbedNormal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.1f), mMaterial.shininess.r);
    vec3 specular = specularStrength * (spec * alightColor * mMaterial.specular);

    vec3 result = (ambient + diffuse + specular) * fragColor;
    float gamma = 2.2f;
    result = pow(result, vec3(1.0f/gamma));

    float depth = LinearizeDepth(gl_FragCoord.z) / 100.0f;
    

    outColor =  texture(texSampler, fragTexCoord) * vec4(result, 1.0f);
    //outColor = vec4(worldNormal * 0.5 + 0.5, 1.0);
    //outColor = depth * vec4(result , 1.0f);

}
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~                                                                                                                                                                                       



/*
        if (ImGui::Button("Refresh"))
        {
            find_files(&scene_paths, "res/data/user/", ".json");
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
        }*/







void thread_function(VkDevice* device, VkPhysicalDevice* physicalDevice, VkQueue graphicsQueue, std::vector<Model*> models, std::vector<VkBuffer>& lightBuffers, std::mutex& queueMutex) {
    VulkanResources resources;

    // Create command pool
    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.queueFamilyIndex = 0; // Replace with actual queue family index
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

    if (vkCreateCommandPool(*device, &poolInfo, nullptr, &resources.commandPool) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create command pool!");
    }

    // Allocate command buffer
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = resources.commandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = 1;

    if (vkAllocateCommandBuffers(*device, &allocInfo, &resources.commandBuffer) != VK_SUCCESS) {
        throw std::runtime_error("Failed to allocate command buffers!");
    }

    // Record commands
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(resources.commandBuffer, &beginInfo);

    // Initialize resources for each model
    for (Model* model : models) {
        init_model_resources(device, physicalDevice, resources, graphicsQueue, model, &lightBuffers);
    }

    vkEndCommandBuffer(resources.commandBuffer);

    // Submit command buffer
    {
        std::lock_guard<std::mutex> lock(queueMutex);
        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &resources.commandBuffer;

        if (vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE) != VK_SUCCESS) {
            throw std::runtime_error("Failed to submit command buffer!");
        }

        // Wait for the queue to idle (optional, depending on your synchronization needs)
        vkQueueWaitIdle(graphicsQueue);
    }

    // Cleanup
    vkFreeCommandBuffers(*device, resources.commandPool, 1, &resources.commandBuffer);
    vkDestroyCommandPool(*device, resources.commandPool, nullptr);
}