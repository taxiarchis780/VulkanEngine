#include "Resource.h"



void create_color_resources(VkDevice* device, VkPhysicalDevice* physicalDevice, it_ImageResource* colorImageRes,VkFormat swapChainImageFormat, VkExtent2D swapChainExtent, VkSampleCountFlagBits msaaSamples)
{
    VkFormat colorFormat = swapChainImageFormat;

    createImage(device, physicalDevice, swapChainExtent.width, swapChainExtent.height, 1, msaaSamples, colorFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, colorImageRes->image, colorImageRes->memory);
    colorImageRes->imageView = createImageView(*device, colorImageRes->image, colorFormat, VK_IMAGE_ASPECT_COLOR_BIT, 1);

}

void create_depth_resources(VkDevice* device, VkPhysicalDevice* physicalDevice, it_ImageResource* depthImageRes, VkFormat swapChainImageFormat, VkExtent2D swapChainExtent, VkSampleCountFlagBits msaaSamples)
{
    VkFormat depthFormat = findDepthFormat(*physicalDevice);

    createImage(device, physicalDevice, swapChainExtent.width, swapChainExtent.height, 1, msaaSamples, depthFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, depthImageRes->image, depthImageRes->memory);
    depthImageRes->imageView = createImageView(*device, depthImageRes->image, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT, 1);
}
