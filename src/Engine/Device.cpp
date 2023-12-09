#ifndef __DEVICE_SRC__
#define __DEVICE_SRC__

#include "Engine.h"

void Engine::pickPhysicalDevice()
{

    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
    std::vector<VkPhysicalDevice> devices(deviceCount);
    VkResult res = vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());
    VkPhysicalDeviceProperties deviceProperties;

    if (deviceCount == 0) {
        throw std::runtime_error("ERROR: failed to find GPUs with Vulkan support!");
    }



    std::multimap<int, VkPhysicalDevice> candidates;

    for (const auto& device : devices)
    {
        int score = 0;
        vkGetPhysicalDeviceProperties(device, &deviceProperties);
        if (isDeviceSuitable(device))
        {
            score = rateDeviceSuitability(device);
            candidates.insert(std::make_pair(score, device));
        }
        //printf("%s with score: %d\n", &deviceProperties.deviceName, score);
        std::stringstream ss;

        ss << deviceProperties.deviceName << " with score: " << score;

        tlog::info(ss.str());
    }



    if (candidates.rbegin()->first > 0) {
        physicalDevice = candidates.rbegin()->second;
        msaaSamples = getMaxUsableSampleCount();
    }
    else {
        throw std::runtime_error("ERROR: failed to find a suitable GPU!");
    }

    //This is done to find out the score

    vkGetPhysicalDeviceProperties(physicalDevice, &deviceProperties);

    //printf("Card: %s selected with score: %d\n", &deviceProperties.deviceName, candidates.rbegin()->first);
    std::stringstream ss;
    RendererName = std::string(deviceProperties.deviceName);
    ss << "Card: " << deviceProperties.deviceName << " selected with score: " << candidates.rbegin()->first;
    tlog::none();
    tlog::info(ss.str());
}
bool Engine::isDeviceSuitable(VkPhysicalDevice device)
{
    QueueFamilyIndices indices = findQueueFamilies(device);
    bool extensionsSupported = checkDeviceExtensionSupport(device);
    bool swapChainAdequate = false;
    VkPhysicalDeviceFeatures supportedFeatures;
    vkGetPhysicalDeviceFeatures(device, &supportedFeatures);
    if (extensionsSupported)
    {
        SwapChainSupportDetails SwapChainSupport = querySwapChainSupport(device);
        swapChainAdequate = !SwapChainSupport.formats.empty() && !SwapChainSupport.presentModes.empty();
    }

    return indices.isComplete() && extensionsSupported && swapChainAdequate && supportedFeatures.samplerAnisotropy;
}
bool Engine::checkDeviceExtensionSupport(VkPhysicalDevice device)
{
    uint32_t extensionCount;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);
    std::vector<VkExtensionProperties> availableExtensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());
    std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());
    for (const auto& extension : availableExtensions)
    {
        requiredExtensions.erase(extension.extensionName);
    }
    return requiredExtensions.empty();
}
int Engine::rateDeviceSuitability(VkPhysicalDevice device)
{
    VkPhysicalDeviceProperties deviceProperties;
    VkPhysicalDeviceFeatures deviceFeatures;
    VkPhysicalDeviceMemoryProperties deviceMemoryProperties;

    vkGetPhysicalDeviceProperties(device, &deviceProperties);
    vkGetPhysicalDeviceFeatures(device, &deviceFeatures);
    vkGetPhysicalDeviceMemoryProperties(device, &deviceMemoryProperties);

    int score = 0;
    // Discrete GPUs have a significant performance advantage
    if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
        score += 1000;
    }
    else if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU)
    {
        score += 100;
    }
    // Maximum possible size of textures affects graphics quality
    score += deviceProperties.limits.maxImageDimension2D;
    // Application can't function without geometry shaders
    if (!deviceFeatures.geometryShader) {
        return 0;
    }
    // https://stackoverflow.com/questions/44339931/query-amount-of-vram-or-gpu-clock-speed
    auto heapsPointer = deviceMemoryProperties.memoryHeaps;
    auto heaps = std::vector<VkMemoryHeap>(heapsPointer, heapsPointer + deviceMemoryProperties.memoryHeapCount);
    //std::cout << deviceProperties.deviceName << std::endl;

    for (const auto& heap : heaps)
    {
        if (heap.flags & VkMemoryHeapFlagBits::VK_MEMORY_HEAP_DEVICE_LOCAL_BIT)
        {
            score += heap.size / 100000;
        }
    }

    score += (deviceProperties.limits.maxVertexInputAttributes + deviceProperties.limits.maxFragmentInputComponents) * 100;

    return score;

}


#endif
