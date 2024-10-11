#ifndef __GRAPHICS_PIPELINE_H__
#define __GRAPHICS_PIPELINE_H__

#include <vulkan/vulkan.h>
#include <vector>
#include <stdexcept>

#include "util.h"


VkShaderModule create_shader_module(VkDevice* device, const std::vector<char>& code);

void create_graphics_pipeline(VkDevice* device, int index, std::string vertShaderPath, std::string fragShaderPath,
    std::vector<VkPipelineLayout>* pipelineLayouts, std::vector<VkPipeline>* graphicsPipelines, VkSampleCountFlagBits msaaSamples,
    VkDescriptorSetLayout descriptorSetLayout, VkRenderPass renderPass);

void create_shadow_pipeline(VkDevice* device, std::string vertShaderPath, std::string fragShaderPath, VkPipelineLayout* shadowPipelineLayout,
    VkPipeline* shadowPipeline, VkDescriptorSetLayout descriptorSetLayout, VkRenderPass shadowRenderPass, VkExtent2D shadowMapExtent, VkSampleCountFlagBits msaaSamples);
#endif