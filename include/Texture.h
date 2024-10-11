#ifndef __TEXTURE_H__
#define __TEXTURE_H__

#include <vulkan/vulkan.h>
#include <stdexcept>

#include "Image.h"
#include "Model.h"
#include "Buffer.h"

// !! TEMPORARY !! MUST NOT BE EXPOSED (using it until normal implimentation is moved here)
void generateMipmaps(VkDevice* device, VkPhysicalDevice* physicalDevice, VkCommandPool commandPool, VkQueue graphicsQueue, VkImage image, VkFormat imageFormat, int32_t texWidth, int32_t texHeight, uint32_t mipLevels);


void create_texture_image(VkDevice* device, VkPhysicalDevice* physicalDevice, VkCommandPool commandPool, VkQueue graphicsQueue, Model* cModel);
void create_imageview(VkDevice* device, Model* cModel);
void create_texture_sampler(VkDevice* device, VkPhysicalDevice* physicalDevice, Model* cModel);

void create_normal_image(VkDevice* device, VkPhysicalDevice* physicalDevice, VkCommandPool commandPool, VkQueue graphicsQueue, Model* cModel);
void create_normalview(VkDevice* device, Model* cModel);
void create_normal_sampler(VkDevice* device, VkPhysicalDevice* physicalDevice, Model* cModel);

#endif