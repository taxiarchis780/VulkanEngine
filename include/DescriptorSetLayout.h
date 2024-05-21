#ifndef __DESCRIPTOR_SET_LAYOUT_H__
#define __DESCRIPTOR_SET_LAYOUT_H__

#include <vulkan/vulkan.h>
#include <array>
#include <stdexcept>

struct it_DescriptorSetLayout
{
	VkDescriptorSetLayout descriptorSetLayout;
};

void create_descriptor_set_layout(VkDevice* device, VkDescriptorSetLayout* descriptorSetLayout);


#endif