#include "Command.h"
#define MAX_FRAMES_IN_FLIGHT 2


VkCommandBuffer beginSingleTimeCommands(VkDevice* device, VkCommandPool commandPool) 
{
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = commandPool;
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer;
    vkAllocateCommandBuffers(*device, &allocInfo, &commandBuffer);

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(commandBuffer, &beginInfo);

    return commandBuffer;
}

void endSingleTimeCommands(VkDevice* device, VkCommandBuffer commandBuffer, VkCommandPool commandPool, VkQueue graphicsQueue) 
{
    vkEndCommandBuffer(commandBuffer);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(graphicsQueue);

    vkFreeCommandBuffers(*device, commandPool, 1, &commandBuffer);
}



void create_command_pool(VkDevice* device, VkPhysicalDevice* physicalDevice, VkCommandPool* commandPool, VkSurfaceKHR* surface)
{
    QueueFamilyIndices queueFamilyIndices = findQueueFamilies(*physicalDevice, *surface);

    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();

    if (vkCreateCommandPool(*device, &poolInfo, nullptr, commandPool) != VK_SUCCESS)
        throw std::runtime_error("ERROR: failed to create commandPool");
}


void create_commandbuffer(VkDevice* device, std::vector<VkCommandBuffer>* commandBuffers, VkCommandPool commandPool)
{
    (*commandBuffers).resize(MAX_FRAMES_IN_FLIGHT);
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = commandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = (uint32_t)(*commandBuffers).size();

    if (vkAllocateCommandBuffers(*device, &allocInfo, (*commandBuffers).data()) != VK_SUCCESS)
    {
        throw std::runtime_error("ERROR: failed to allocate command buffers!");
    }
}