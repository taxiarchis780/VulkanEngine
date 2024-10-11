#ifndef __SYNC_OBJECT_H__
#define __SYNC_OBJECT_H__

#include <vulkan/vulkan.h>
#include <stdexcept>
#include <vector>


void create_sync_objects(VkDevice* device, std::vector<VkSemaphore>* imageAvailableSemaphores, std::vector<VkSemaphore>* renderFinishedSemaphores, std::vector<VkFence>* inFlightFences);

#endif