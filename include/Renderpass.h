#ifndef __RENDER_PASS_H__
#define __RENDER_PASS_H__

#include <vulkan/vulkan.h>
#include <array>
#include <stdexcept>
#include <vector>

#include "Image.h"


struct it_RenderPass
{
	VkRenderPass renderPass;
};



void create_render_pass(VkDevice* device, VkPhysicalDevice* physicalDevice, VkRenderPass* renderPass, VkFormat swapChainImageFormat, VkSampleCountFlagBits msaaSamples);

#endif