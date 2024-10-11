#ifndef __DESCRIPTOR_SET_H__
#define __DESCRIPTOR_SET_H__

#include <vulkan/vulkan.h>
#include <array>
#include <stdexcept>

#include "Model.h"
#include "ResourceBuffer.h"

struct it_DescriptorSetLayout
{
	VkDescriptorSetLayout descriptorSetLayout;
};


void create_descriptor_set_layout(VkDevice* device, VkDescriptorSetLayout* descriptorSetLayout);

void create_descriptor_set_layout(VkDevice* device, Model* cModel);

void create_descriptor_pool(VkDevice* device, Model* cModel);

void create_descriptor_sets(VkDevice* device, Model* cModel, it_ImageResource* shadowRes, std::vector<VkBuffer> lightBuffers);

void update_descriptor_set(VkDevice device, VkDescriptorSet descriptorSet, VkImageView imageView, VkSampler sampler, VkImageLayout layout, uint32_t binding);
#endif