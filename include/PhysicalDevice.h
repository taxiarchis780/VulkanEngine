#ifndef __PHYSICAL_DEVICE_H__
#define __PHYSICAL_DEVICE_H__

#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>
#include <tinylogger.h>
#include <stdexcept>
#include <vector>
#include <map>
#include <sstream>

#include "QueueFamily.h"
#include "SwapChain.h"


struct it_PhysicalDevice {
	VkPhysicalDevice physicalDevice;
};

const std::vector<const char*> deviceExtensions =
{
    VK_KHR_SWAPCHAIN_EXTENSION_NAME
    //VK_NV_RAY_TRACING_EXTENSION_NAME
};

void pick_physical_device(VkPhysicalDevice* physicalDevice, VkInstance* instance, VkSurfaceKHR* surface, VkSampleCountFlagBits* msaaSamples, std::string* RendererName);


#endif
