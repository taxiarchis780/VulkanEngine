#ifndef __COMMAND_H__
#define __COMMAND_H__

#include <vulkan/vulkan.h>
#include <stdexcept>
#include <vector>

#include "QueueFamily.h"

struct it_CommandPool {
	VkCommandPool commandPool;
};


VkCommandBuffer beginSingleTimeCommands(VkDevice* device, VkCommandPool commandPool);
void endSingleTimeCommands(VkDevice* device, VkCommandBuffer commandBuffer, VkCommandPool commandPool, VkQueue graphicsQueue);


void create_command_pool(VkDevice* device, VkPhysicalDevice* physicalDevice, VkCommandPool* commandPool, VkSurfaceKHR* surface);

void create_commandbuffer(VkDevice* device, std::vector<VkCommandBuffer>* commandBuffers, VkCommandPool commandPool);

#endif