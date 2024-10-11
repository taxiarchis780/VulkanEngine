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

    createImage(device, physicalDevice, swapChainExtent.width, swapChainExtent.height, 1, msaaSamples, depthFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, depthImageRes->image, depthImageRes->memory);
    depthImageRes->imageView = createImageView(*device, depthImageRes->image, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT, 1);


    VkSamplerCreateInfo samplerInfo = {};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.anisotropyEnable = VK_FALSE;
    samplerInfo.maxAnisotropy = 1.0f;
    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    samplerInfo.compareEnable = VK_TRUE;
    samplerInfo.compareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.minLod = 0.0f;
    samplerInfo.maxLod = 1.0f;
    samplerInfo.mipLodBias = 0.0f;

    if (vkCreateSampler(*device, &samplerInfo, nullptr, &depthImageRes->sampler) != VK_SUCCESS) {
        throw std::runtime_error("failed to create shadow map sampler!");
    }

}

void create_shadow_resources(VkDevice* device, VkPhysicalDevice* physicalDevice, it_ImageResource* shadowImageRes, VkFormat swapChainImageFormat, VkExtent2D swapChainExtent, VkSampleCountFlagBits msaaSamples)
{
    // Create the depth image for the shadow map
    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = swapChainExtent.width;
    imageInfo.extent.height = swapChainExtent.height;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.format = VK_FORMAT_D32_SFLOAT;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateImage(*device, &imageInfo, nullptr, &shadowImageRes->image) != VK_SUCCESS) {
        throw std::runtime_error("failed to create shadow map image!");
    }

    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(*device, shadowImageRes->image, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryType(*physicalDevice, memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    if (vkAllocateMemory(*device, &allocInfo, nullptr, &shadowImageRes->memory) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate shadow map image memory!");
    }

    vkBindImageMemory(*device, shadowImageRes->image, shadowImageRes->memory, 0);

    // Create the image view for the depth image
    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = shadowImageRes->image;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = VK_FORMAT_D32_SFLOAT;
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;

    if (vkCreateImageView(*device, &viewInfo, nullptr, &shadowImageRes->imageView) != VK_SUCCESS) {
        throw std::runtime_error("failed to create shadow map image view!");
    }

    // Create the sampler for the shadow map
    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.anisotropyEnable = VK_FALSE;
    samplerInfo.maxAnisotropy = 1.0f;
    samplerInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    samplerInfo.compareEnable = VK_TRUE;
    samplerInfo.compareOp = VK_COMPARE_OP_LESS;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.mipLodBias = 0.0f;
    samplerInfo.minLod = 0.0f;
    samplerInfo.maxLod = 1.0f;

    if (vkCreateSampler(*device, &samplerInfo, nullptr, &shadowImageRes->sampler) != VK_SUCCESS) {
        throw std::runtime_error("failed to create shadow map sampler!");
    }
}



