#ifndef __BUFFER_H__
#define __BUFFER_H__

#include <vulkan/vulkan.h>
#include <stdexcept>

#include "Image.h"
#include "Command.h"


struct it_Buffer {
	VkBuffer buffer;
};

void copy_buffer(VkDevice* device, VkCommandPool commandPool, VkQueue graphicsQueue, VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
void copy_buffer_to_image(VkDevice* device, VkCommandPool commandPool, VkQueue graphicsQueue, VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);


void create_buffer(VkDevice* device, VkPhysicalDevice* physicalDevice, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory);

#endif