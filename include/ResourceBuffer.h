#ifndef __RESOURCE_BUFFER_H__
#define __RESOURCE_BUFFER_H__

#include <vulkan/vulkan.h>
#include <stdexcept>

#include "Model.h"
#include "Buffer.h"


struct UniformBufferObject
{
	glm::mat4 transform;
	glm::mat4 view;
	glm::mat4 proj;
	glm::mat4 lightSpaceMat;
	glm::vec3 cameraPos;
	Material material;
	float time;
};

struct LightsUniformBufferObject
{
	glm::vec3 lightPos = glm::vec3(0.0f);
	glm::vec3 lightRot = glm::vec3(0.0f);
	glm::vec3 lightColor = glm::vec3(0.0f);
};

struct it_lightBufferResource
{
	std::vector<VkBuffer> lightBuffers;
	std::vector<VkDeviceMemory> lightBuffersMemory;
	std::vector<void*> lightBuffersMapped;
};

void create_vertex_buffer(VkDevice* device, VkPhysicalDevice* physicalDevice, VkCommandPool commandPool, VkQueue graphicsQueue, Model* cModel);

void create_index_buffer(VkDevice* device, VkPhysicalDevice* physicalDevice, VkCommandPool commandPool, VkQueue graphicsQueue, Model* cModel);

void create_uniform_buffers(VkDevice* device, VkPhysicalDevice* physicalDevice, Model* cModel);

void create_light_uniform_buffer(VkDevice* device, VkPhysicalDevice* physicalDevice, it_lightBufferResource* lightRes);

void cleanup_light_uniform_buffer(VkDevice device, it_lightBufferResource* lightRes);

#endif