#ifndef __RESOURCE_H__
#define __RESOURCE_H__

#include <vulkan/vulkan.h>
#include <stdexcept>

#include "Image.h"

void create_color_resources(VkDevice* device, VkPhysicalDevice* physicalDevice, it_ImageResource* colorImageRes, VkFormat swapChainImageFormat, VkExtent2D swapChainExtent, VkSampleCountFlagBits msaaSamples);

void create_depth_resources(VkDevice* device, VkPhysicalDevice* physicalDevice, it_ImageResource* depthImageRes, VkFormat swapChainImageFormat, VkExtent2D swapChainExtent, VkSampleCountFlagBits msaaSamples);

#endif