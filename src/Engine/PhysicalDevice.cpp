#include <PhysicalDevice.h>

bool checkDeviceExtensionSupport(VkPhysicalDevice device)
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


bool isDeviceSuitable(VkPhysicalDevice device, VkSurfaceKHR surface)
{
    QueueFamilyIndices indices = findQueueFamilies(device, surface);
    bool extensionsSupported = checkDeviceExtensionSupport(device);
    bool swapChainAdequate = false;
    VkPhysicalDeviceFeatures supportedFeatures;
    vkGetPhysicalDeviceFeatures(device, &supportedFeatures);
    if (extensionsSupported)
    {
        SwapChainSupportDetails SwapChainSupport = querySwapChainSupport(device, surface);
        swapChainAdequate = !SwapChainSupport.formats.empty() && !SwapChainSupport.presentModes.empty();
    }

    return indices.isComplete() && extensionsSupported && swapChainAdequate && supportedFeatures.samplerAnisotropy;
}




int rateDeviceSuitability(VkPhysicalDevice device)
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


VkSampleCountFlagBits getMaxUsableSampleCount(VkPhysicalDevice physicalDevice)
{
    VkPhysicalDeviceProperties physicalDeviceProperties;
    vkGetPhysicalDeviceProperties(physicalDevice, &physicalDeviceProperties);

    VkSampleCountFlags counts = physicalDeviceProperties.limits.framebufferColorSampleCounts & physicalDeviceProperties.limits.framebufferDepthSampleCounts;
    if (counts & VK_SAMPLE_COUNT_64_BIT) { return VK_SAMPLE_COUNT_64_BIT; }
    if (counts & VK_SAMPLE_COUNT_32_BIT) { return VK_SAMPLE_COUNT_32_BIT; }
    if (counts & VK_SAMPLE_COUNT_16_BIT) { return VK_SAMPLE_COUNT_16_BIT; }
    if (counts & VK_SAMPLE_COUNT_8_BIT) { return VK_SAMPLE_COUNT_8_BIT; }
    if (counts & VK_SAMPLE_COUNT_4_BIT) { return VK_SAMPLE_COUNT_4_BIT; }
    if (counts & VK_SAMPLE_COUNT_2_BIT) { return VK_SAMPLE_COUNT_2_BIT; }

    return VK_SAMPLE_COUNT_1_BIT;
}


void pick_physical_device(VkPhysicalDevice* physicalDevice, VkInstance* instance, VkSurfaceKHR* surface, VkSampleCountFlagBits* msaaSamples, std::string* RendererName)
{

    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(*instance, &deviceCount, nullptr);
    std::vector<VkPhysicalDevice> devices(deviceCount);
    VkResult res = vkEnumeratePhysicalDevices(*instance, &deviceCount, devices.data());
    VkPhysicalDeviceProperties deviceProperties;

    if (deviceCount == 0) {
        throw std::runtime_error("ERROR: failed to find GPUs with Vulkan support!");
    }



    std::multimap<int, VkPhysicalDevice> candidates;

    for (const auto& device : devices)
    {
        int score = 0;
        vkGetPhysicalDeviceProperties(device, &deviceProperties);
        if (isDeviceSuitable(device, *surface))
        {
            score = rateDeviceSuitability(device);
            candidates.insert(std::make_pair(score, device));
        }
        //printf("%s with score: %d\n", &deviceProperties.deviceName, score);
        std::stringstream ss;

        ss << deviceProperties.deviceName << " with score: " << score;
#ifndef ENGINE_DISABLE_LOGGING
        tlog::info(ss.str());
#endif
    }



    if (candidates.rbegin()->first > 0) {
        *physicalDevice = candidates.rbegin()->second;
        *msaaSamples = getMaxUsableSampleCount(*physicalDevice);
    }
    else {
        throw std::runtime_error("ERROR: failed to find a suitable GPU!");
    }

    //This is done to find out the score

    vkGetPhysicalDeviceProperties(*physicalDevice, &deviceProperties);

    //printf("Card: %s selected with score: %d\n", &deviceProperties.deviceName, candidates.rbegin()->first);

    std::stringstream ss;
    *RendererName = std::string(deviceProperties.deviceName);

    ss << "Card: " << deviceProperties.deviceName << " selected with score: " << candidates.rbegin()->first;
#ifndef ENGINE_DISABLE_LOGGING
    tlog::none();
    tlog::info(ss.str());
#endif
}