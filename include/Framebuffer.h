#ifndef __FRAMEBUFFER_H__
#define __FRAMEBUFFER_H__

#include <vulkan/vulkan.h>
#include "GLFW/glfw3.h"
#include "GLFW/glfw3native.h"
#include <stdexcept>
#include <vector>
#include <array>

struct it_Framebuffer {
	VkFramebuffer frameBuffer;
};




void create_framebuffers(VkDevice* device, std::vector<VkFramebuffer>* swapChainFramebuffers, std::vector<VkImageView> swapChainImageViews, VkExtent2D swapChainExtent, VkRenderPass renderPass, VkImageView colorImageView, VkImageView depthImageView);
void create_shadow_framebuffer(VkDevice* device, VkFramebuffer* shadowFramebuffer, VkImageView* depthImageView, VkRenderPass* shadowRenderPass, VkExtent2D shadowExtent);

#endif