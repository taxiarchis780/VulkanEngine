#include "SwapChain.h"

SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device, VkSurfaceKHR surface) {
    SwapChainSupportDetails details;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);

    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);

    if (formatCount != 0) {
        details.formats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.formats.data());
    }

    uint32_t presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);

    if (presentModeCount != 0) {
        details.presentModes.resize(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, details.presentModes.data());
    }

    return details;
}

VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats)
{
    for (const auto& availableFormat : availableFormats)
    {
        if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
        {
            return availableFormat;
        }
    }
    return availableFormats[0];
}

VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes, VkPresentModeKHR presentMode)
{
    for (const auto& availablePresentMode : availablePresentModes)
    {
        if (availablePresentMode == presentMode) {

            return availablePresentMode;
        }
    }

    return VK_PRESENT_MODE_FIFO_KHR; //Shitty Vsync
}

VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities, GLFWwindow* window)
{
    if (capabilities.currentExtent.width != (std::numeric_limits<uint32_t>::max)()) { return capabilities.currentExtent; }
    else {
        int width, height;
        glfwGetFramebufferSize(window, &width, &height);

        VkExtent2D actualExtent = {
            static_cast<uint32_t>(width),
            static_cast<uint32_t>(height)
        };

        actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
        actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

        return actualExtent;
    }
}



void create_swapchain(VkDevice* device, VkPhysicalDevice* physicalDevice, it_SwapChainHandle* swapChainHandle, VkSurfaceKHR* surface, GLFWwindow* window, bool VSync)
{
    
    SwapChainSupportDetails swapChainSupport = querySwapChainSupport(*physicalDevice, *surface);
    VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
    VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes, swapChainHandle->presentMode);
    if (presentMode == VK_PRESENT_MODE_FIFO_KHR || presentMode == VK_PRESENT_MODE_FIFO_RELAXED_KHR)
    {
        VSync = true;
    }
    else {
        VSync = false;
    }

    

    VkExtent2D extent = chooseSwapExtent(swapChainSupport.capabilities, window);

    uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
    if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount) { imageCount = swapChainSupport.capabilities.maxImageCount; }

    VkSwapchainCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = *surface;
    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageExtent = extent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    QueueFamilyIndices indices = findQueueFamilies(*physicalDevice, *surface);
    uint32_t QueueFamilyIndices[] = { indices.graphicsFamily.value(), indices.presentFamily.value() };

    if (indices.graphicsFamily != indices.presentFamily) {
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = QueueFamilyIndices;
    }
    else {
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        createInfo.queueFamilyIndexCount = 0;
        createInfo.pQueueFamilyIndices = nullptr;
    }

    createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode = presentMode;
    createInfo.clipped = VK_TRUE;
    createInfo.oldSwapchain = VK_NULL_HANDLE;

    if (vkCreateSwapchainKHR(*device, &createInfo, nullptr, &swapChainHandle->swapChain) != VK_SUCCESS)
    {
        throw std::runtime_error("ERROR: failed to create swap chain!");
    }

    vkGetSwapchainImagesKHR(*device, swapChainHandle->swapChain, &imageCount, nullptr);
    swapChainHandle->images.resize(imageCount);
    vkGetSwapchainImagesKHR(*device, swapChainHandle->swapChain, &imageCount, swapChainHandle->images.data());
    swapChainHandle->imageFormat = surfaceFormat.format;
    swapChainHandle->extent = extent;
}


void cleanupSwapChain(VkDevice* device, it_SwapChainHandle* swapChainHandle, it_ImageResource* colorImageRes, it_ImageResource* depthImageRes)
{
    vkDestroyImageView(*device, colorImageRes->imageView, nullptr);
    vkDestroyImage(*device, colorImageRes->image, nullptr);
    vkFreeMemory(*device, colorImageRes->memory, nullptr);
    vkDestroyImageView(*device, depthImageRes->imageView, nullptr);
    vkDestroyImage(*device, depthImageRes->image, nullptr);
    vkFreeMemory(*device, depthImageRes->memory, nullptr);
    for (size_t i = 0; i < swapChainHandle->framebuffers.size(); i++)
        vkDestroyFramebuffer(*device, swapChainHandle->framebuffers[i], nullptr);
    for (size_t i = 0; i < swapChainHandle->imageViews.size(); i++)
        vkDestroyImageView(*device, swapChainHandle->imageViews[i], nullptr);
    vkDestroySwapchainKHR(*device, swapChainHandle->swapChain, nullptr);
}


void recreate_swapchain(VkDevice* device, VkPhysicalDevice* physicalDevice, it_SwapChainHandle* swapChainHandle, it_ImageResource* colorImageRes, it_ImageResource* depthImageRes, VkSurfaceKHR* surface, VkRenderPass* renderPass, VkSampleCountFlagBits msaaSamples, GLFWwindow* window, Camera* camera, bool VSync)
{
    int width = 0, height = 0;
    glfwGetFramebufferSize(window, &width, &height);
    while (width == 0 || height == 0)
    {
        glfwGetFramebufferSize(window, &width, &height);
        glfwWaitEvents();
    }

    vkDeviceWaitIdle(*device);


    cleanupSwapChain(device, swapChainHandle, colorImageRes, depthImageRes);
    create_swapchain(device, physicalDevice, swapChainHandle, surface, window, VSync);

    camera->width = (*swapChainHandle).extent.width;
    camera->height = (*swapChainHandle).extent.height;
    create_imageviews(device, &swapChainHandle->imageViews, &swapChainHandle->images, swapChainHandle->imageFormat);
    create_color_resources(device, physicalDevice, colorImageRes, swapChainHandle->imageFormat, swapChainHandle->extent, msaaSamples);
    create_depth_resources(device, physicalDevice, depthImageRes, swapChainHandle->imageFormat, swapChainHandle->extent, msaaSamples);
    create_framebuffers(device, &swapChainHandle->framebuffers, swapChainHandle->imageViews, swapChainHandle->extent, *renderPass, colorImageRes->imageView, depthImageRes->imageView);

}

