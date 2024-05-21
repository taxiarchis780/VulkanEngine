#ifndef __IMAGE_H__
#define __IMAGE_H__

#include <vulkan/vulkan.h>
#include <stdexcept>
#include <vector>

struct it_ImageResource
{
	VkImageView		imageView;
	VkImage			image;
	VkDeviceMemory  memory;
};

VkFormat findDepthFormat(VkPhysicalDevice physicalDevice);

uint32_t findMemoryType(VkPhysicalDevice physicalDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties);

void createImage(VkDevice* device, VkPhysicalDevice* physicalDevice, uint32_t width, uint32_t height, uint32_t mipLevels, VkSampleCountFlagBits numSamples, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory);

VkImageView createImageView(VkDevice device, VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, uint32_t mipLevels);

void create_imageviews(VkDevice* device, std::vector<VkImageView>* swapChainImageViews, std::vector<VkImage>* swapChainImages, VkFormat swapChainImageFormat);

#endif
