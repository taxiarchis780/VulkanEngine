#include <Model.h>

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

#include <assimp/Importer.hpp>

#include "Texture.h"
#include "ResourceBuffer.h"
#include "DescriptorSet.h"


void init_model(Model* cModel, std::string MODEL_PATH, std::string TEXTURE_PATH)
{
    cModel->MODEL_PATH = MODEL_PATH;
    cModel->TEXTURE_PATH = TEXTURE_PATH;
    cModel->NORMAL_PATH = std::string("textures/neutral_normal.jpg");
}

void cleanup_model(VkDevice* device, Model* cModel)
{
   
    for (auto& ubo : cModel->uniformBuffers)
        vkDestroyBuffer(*device, ubo, nullptr);
    for (auto& mem : cModel->uniformBuffersMemory)
        vkFreeMemory(*device, mem, nullptr);
    vkDestroyDescriptorPool(*device, cModel->descriptorPool, nullptr);
    if (!cModel->NORMAL_PATH.empty())
    {
        vkDestroySampler(*device, cModel->normalSampler, nullptr);
        vkDestroyImageView(*device, cModel->normalImageView, nullptr);
        vkDestroyImage(*device, cModel->normalImage, nullptr);
        vkFreeMemory(*device, cModel->normalImageMemory, nullptr);
        vkDestroyDescriptorSetLayout(*device, cModel->descriptorSetLayout, nullptr);
    }

    vkDestroySampler(*device, cModel->textureSampler, nullptr);
    vkDestroyImageView(*device, cModel->textureImageView, nullptr);
    vkDestroyImage(*device, cModel->textureImage, nullptr);
    vkFreeMemory(*device, cModel->textureImageMemory, nullptr);
    vkDestroyBuffer(*device, cModel->vertexBuffer, nullptr);

    vkFreeMemory(*device, cModel->vertexBufferMemory, nullptr);
    vkDestroyBuffer(*device, cModel->indexBuffer, nullptr);
    vkFreeMemory(*device, cModel->indexBufferMemory, nullptr);

    

    free(cModel);
}


void load_model(Model* cModel) {
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string err;
    

    if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &err, (cModel->baseDir + cModel->MODEL_PATH).c_str(), "res/models/")) {
        throw std::runtime_error(err);
    }
  
    std::unordered_map<Vertex, uint32_t> uniqueVertices{};
    std::vector<glm::vec3> tangents{};
    std::vector<glm::vec3> bitangents{};


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
                uniqueVertices[vertex] = static_cast<uint32_t>(cModel->vertices.size());
                cModel->vertices.push_back(vertex);
                tangents.push_back(glm::vec3(0.0f));
                bitangents.push_back(glm::vec3(0.0f));
            }


            cModel->indices.push_back(uniqueVertices[vertex]);

        }
#ifndef ENGINE_DISABLE_LOGGING
        prog++;
        std::this_thread::sleep_for(std::chrono::milliseconds{ 100 });
        progress.update(prog);
#endif

    }
    for (size_t i = 0; i < cModel->indices.size(); i += 3) {
        Vertex& v0 = cModel->vertices[cModel->indices[i + 0]];
        Vertex& v1 = cModel->vertices[cModel->indices[i + 1]];
        Vertex& v2 = cModel->vertices[cModel->indices[i + 2]];

        glm::vec3 edge1 = v1.pos - v0.pos;
        glm::vec3 edge2 = v2.pos - v0.pos;

        glm::vec2 deltaUV1 = v1.texCoord - v0.texCoord;
        glm::vec2 deltaUV2 = v2.texCoord - v0.texCoord;

        float f = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y);
        // Check for division by zero
        if (!std::isfinite(f)) {
            f = 0.0f;
        }

        glm::vec3 tangent, bitangent;

        tangent.x = f * (deltaUV2.y * edge1.x - deltaUV1.y * edge2.x);
        tangent.y = f * (deltaUV2.y * edge1.y - deltaUV1.y * edge2.y);
        tangent.z = f * (deltaUV2.y * edge1.z - deltaUV1.y * edge2.z);
        tangent = glm::normalize(tangent);

        bitangent.x = f * (-deltaUV2.x * edge1.x + deltaUV1.x * edge2.x);
        bitangent.y = f * (-deltaUV2.x * edge1.y + deltaUV1.x * edge2.y);
        bitangent.z = f * (-deltaUV2.x * edge1.z + deltaUV1.x * edge2.z);
        bitangent = glm::normalize(bitangent);

        if (glm::dot(glm::cross(v0.normal, tangent), bitangent) < 0.0f) {
            tangent = tangent * -1.0f;
        }

        tangents[cModel->indices[i + 0]] += tangent;
        tangents[cModel->indices[i + 1]] += tangent;
        tangents[cModel->indices[i + 2]] += tangent;

        bitangents[cModel->indices[i + 0]] += bitangent;
        bitangents[cModel->indices[i + 1]] += bitangent;
        bitangents[cModel->indices[i + 2]] += bitangent;
    }

    for (size_t i = 0; i < cModel->vertices.size(); i++) {
        cModel->vertices[i].tangent = glm::normalize(tangents[i]);
        cModel->vertices[i].bitangent = glm::normalize(bitangents[i]);
    }


    cModel->material.ambient = glm::vec3(materials[0].ambient[0], materials[0].ambient[1], materials[0].ambient[2]);
    cModel->material.diffuse = glm::vec3(materials[0].diffuse[0], materials[0].diffuse[1], materials[0].diffuse[2]);
    cModel->material.specular = glm::vec3(materials[0].specular[0], materials[0].specular[1], materials[0].specular[2]);
    cModel->material.shininess = glm::vec3(materials[0].shininess, 0.0f, 0.0f);
    cModel->material.overrideColor = glm::vec3(1.0f, 1.0f, 1.0f);
    //cModel->NORMAL_PATH = "res/textures/" + materials[0].normal_texname;
    
    cModel->statsFaces = static_cast<int>((cModel->indices.size() / 3));
#ifndef ENGINE_DISABLE_LOGGING
    printf("\n");
    tlog::none();
    tlog::success();
    printf("Material count: %d \n", static_cast<int>(materials.size()));
#endif
}


//void init_model_resources(VkDevice* device, VkPhysicalDevice* physicalDevice, VkCommandPool commandPool, VkQueue graphicsQueue, VkDescriptorSetLayout descriptorSetLayout, Model* cModel) 
void init_model_resources(VkDevice* device, VkPhysicalDevice* physicalDevice, VkCommandPool commandPool, VkQueue graphicsQueue, Model* cModel,it_ImageResource* depthRes, std::vector<VkBuffer>* lightBuffers)
{
    create_descriptor_set_layout(device, cModel);

    create_texture_image(device, physicalDevice, commandPool, graphicsQueue, cModel);
    create_imageview(device, cModel);
    create_texture_sampler(device, physicalDevice, cModel);
    
    create_normal_image(device, physicalDevice, commandPool, graphicsQueue, cModel);
    create_normalview(device, cModel);
    create_normal_sampler(device, physicalDevice, cModel);

    
    create_vertex_buffer(device, physicalDevice, commandPool, graphicsQueue, cModel);
    create_index_buffer(device, physicalDevice, commandPool, graphicsQueue, cModel);
    create_uniform_buffers(device, physicalDevice, cModel);
    create_descriptor_pool(device, cModel);
    create_descriptor_sets(device, cModel, depthRes, *lightBuffers);

    return;
}

void mt_init_model_resources(VkDevice* device, VkPhysicalDevice* physicalDevice, VkCommandPool commandPool, VkQueue graphicsQueue, Model* cModel, it_ImageResource* depthRes, std::vector<VkBuffer>* lightBuffers, std::mutex& queueMutex)
{
    create_descriptor_set_layout(device, cModel);
    {
        std::lock_guard<std::mutex> lock(queueMutex);
        create_texture_image(device, physicalDevice, commandPool, graphicsQueue, cModel);
    }
    create_imageview(device, cModel);
    create_texture_sampler(device, physicalDevice, cModel);
    {
        std::lock_guard<std::mutex> lock(queueMutex);
        create_normal_image(device, physicalDevice, commandPool, graphicsQueue, cModel);
    }
    create_normalview(device, cModel);
    create_normal_sampler(device, physicalDevice, cModel);
    {
        std::lock_guard<std::mutex> lock(queueMutex);
        create_vertex_buffer(device, physicalDevice, commandPool, graphicsQueue, cModel);
        create_index_buffer(device, physicalDevice, commandPool, graphicsQueue, cModel);
    }
    create_uniform_buffers(device, physicalDevice, cModel);
    create_descriptor_pool(device, cModel);
    create_descriptor_sets(device, cModel, depthRes ,*lightBuffers);

};


void draw_model(Model* cModel, VkCommandBuffer commandBuffer, VkPipelineLayout graphicsPipelineLayout, int currentFrame) {
    VkDeviceSize offsets[] = { 0 };

    vkCmdBindVertexBuffers(commandBuffer, 0, 1, &cModel->vertexBuffer, offsets);

    vkCmdBindIndexBuffer(commandBuffer, cModel->indexBuffer, 0, VK_INDEX_TYPE_UINT32);

    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipelineLayout, 0, 1, &cModel->descriptorSets[currentFrame], 0, nullptr);

    vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(cModel->indices.size()), 1, 0, 0, 0);

}

