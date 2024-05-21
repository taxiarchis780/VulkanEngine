#ifndef __LOGICAL_DEVICE_H__
#define __LOGICAL_DEVICE_H__

#include <vulkan/vulkan.h>
#include <stdexcept>
#include <vector>
#include <set>

#include "QueueFamily.h"
#include "PhysicalDevice.h"
#include "Instance.h"


struct it_Device {
	VkDevice device;
};

void create_logical_device(VkDevice* device, VkPhysicalDevice* physicalDevice, VkSurfaceKHR* surface, VkQueue* graphicsQueue, VkQueue* presentQueue);

#endif

