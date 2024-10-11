#include "ResourceBuffer.h"
#define MAX_FRAMES_IN_FLIGHT 2

void create_vertex_buffer(VkDevice* device, VkPhysicalDevice* physicalDevice, VkCommandPool commandPool, VkQueue graphicsQueue, Model* cModel)
{
    VkDeviceSize bufferSize = sizeof(cModel->vertices[0]) * cModel->vertices.size();

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    create_buffer(device, physicalDevice, bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

    void* data;
    vkMapMemory(*device, stagingBufferMemory, 0, bufferSize, 0, &data);
    memcpy(data, cModel->vertices.data(), (size_t)bufferSize);
    vkUnmapMemory(*device, stagingBufferMemory);

    create_buffer(device, physicalDevice, bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, cModel->vertexBuffer, cModel->vertexBufferMemory);

    copy_buffer(device, commandPool, graphicsQueue, stagingBuffer, cModel->vertexBuffer, bufferSize);

    vkDestroyBuffer(*device, stagingBuffer, nullptr);
    vkFreeMemory(*device, stagingBufferMemory, nullptr);
}

void create_index_buffer(VkDevice* device, VkPhysicalDevice* physicalDevice, VkCommandPool commandPool, VkQueue graphicsQueue, Model* cModel)
{
    VkDeviceSize bufferSize = sizeof(cModel->indices[0]) * cModel->indices.size();

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    create_buffer(device, physicalDevice, bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

    void* data;
    vkMapMemory(*device, stagingBufferMemory, 0, bufferSize, 0, &data);
    memcpy(data, cModel->indices.data(), (size_t)bufferSize);
    vkUnmapMemory(*device, stagingBufferMemory);

    create_buffer(device, physicalDevice, bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, cModel->indexBuffer, cModel->indexBufferMemory);
    copy_buffer(device, commandPool, graphicsQueue, stagingBuffer, cModel->indexBuffer, bufferSize);
    vkDestroyBuffer(*device, stagingBuffer, nullptr);
    vkFreeMemory(*device, stagingBufferMemory, nullptr);
}

void create_uniform_buffers(VkDevice* device, VkPhysicalDevice* physicalDevice, Model* cModel)
{
    VkDeviceSize bufferSize = sizeof(UniformBufferObject);

    cModel->uniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);
    cModel->uniformBuffersMemory.resize(MAX_FRAMES_IN_FLIGHT);
    cModel->uniformBuffersMapped.resize(MAX_FRAMES_IN_FLIGHT);

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        create_buffer(device, physicalDevice, bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, cModel->uniformBuffers[i], cModel->uniformBuffersMemory[i]);

        vkMapMemory(*device, cModel->uniformBuffersMemory[i], 0, bufferSize, 0, &cModel->uniformBuffersMapped[i]);
    }
}

void create_light_uniform_buffer(VkDevice* device, VkPhysicalDevice* physicalDevice, it_lightBufferResource* lightRes)
{
    
    VkDeviceSize bufferSize = sizeof(LightsUniformBufferObject);

    lightRes->lightBuffers.resize(MAX_FRAMES_IN_FLIGHT);
    lightRes->lightBuffersMemory.resize(MAX_FRAMES_IN_FLIGHT);
    lightRes->lightBuffersMapped.resize(MAX_FRAMES_IN_FLIGHT);

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        create_buffer(device, physicalDevice, bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, (lightRes->lightBuffers)[i], (lightRes->lightBuffersMemory)[i]);

        vkMapMemory(*device, lightRes->lightBuffersMemory[i], 0, bufferSize, 0, &(lightRes->lightBuffersMapped)[i]);
    }
}


void cleanup_light_uniform_buffer(VkDevice device, it_lightBufferResource* lightRes)
{
    for (auto& buff : lightRes->lightBuffers)
    {
        vkDestroyBuffer(device, buff, nullptr);
    }
    for (auto& mem : lightRes->lightBuffersMemory)
    {
        vkFreeMemory(device, mem, nullptr);
    }
    return;
}