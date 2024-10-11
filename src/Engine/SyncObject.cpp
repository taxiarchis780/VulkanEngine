#include "SyncObject.h"
#define MAX_FRAMES_IN_FLIGHT 2

void create_sync_objects(VkDevice* device, std::vector<VkSemaphore>* imageAvailableSemaphores, std::vector<VkSemaphore>* renderFinishedSemaphores, std::vector<VkFence>* inFlightFences)
{
    imageAvailableSemaphores->resize(MAX_FRAMES_IN_FLIGHT);
    renderFinishedSemaphores->resize(MAX_FRAMES_IN_FLIGHT);
    inFlightFences->resize(MAX_FRAMES_IN_FLIGHT);

    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        if (vkCreateSemaphore(*device, &semaphoreInfo, nullptr, &(*imageAvailableSemaphores)[i]) != VK_SUCCESS || vkCreateSemaphore(*device, &semaphoreInfo, nullptr, &(*renderFinishedSemaphores)[i]) != VK_SUCCESS || vkCreateFence(*device, &fenceInfo, nullptr, &(*inFlightFences)[i]) != VK_SUCCESS) {
            throw std::runtime_error("ERROR: failed to create synchronization objects for a frame!");
        }
    }
}