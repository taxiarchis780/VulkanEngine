#ifndef __SWAP_CHAIN_H__
#define __SWAP_CHAIN_H__

#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>
#include <vector>
#include <stdexcept>
#include <algorithm>

#include "QueueFamily.h"
#include "Image.h"
#include "Resource.h"
#include "Framebuffer.h"
#include "Camera.h"


struct SwapChainSupportDetails {
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};

struct it_SwapChainHandle {
    VkSwapchainKHR              swapChain;
    VkFormat                    imageFormat;
    VkExtent2D                  extent;
    VkPresentModeKHR            presentMode = VK_PRESENT_MODE_MAILBOX_KHR;
    std::vector<VkImage>        images;
    std::vector<VkImageView>    imageViews;
    std::vector<VkFramebuffer>  framebuffers;
};


SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device, VkSurfaceKHR surface);

void cleanupSwapChain(VkDevice* device, it_SwapChainHandle* swapChainHandle, it_ImageResource* colorImageRes, it_ImageResource* depthImageRes);


void create_swapchain(VkDevice* device, VkPhysicalDevice* physicalDevice, it_SwapChainHandle* swapChainHandle,VkSurfaceKHR* surface, GLFWwindow* window, bool VSync);
void recreate_swapchain(VkDevice* device, VkPhysicalDevice* physicalDevice, it_SwapChainHandle* swapChainHandle, it_ImageResource* colorImageRes, it_ImageResource* depthImageRes, VkSurfaceKHR* surface, VkRenderPass* renderPass, VkSampleCountFlagBits msaaSamples, GLFWwindow* window, Camera* camera, bool VSync);
#endif
