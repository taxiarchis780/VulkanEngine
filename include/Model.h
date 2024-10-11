#pragma once
#define __MODEL_CLASS__
#include <Vertex.h>
#include <Image.h>
#include <string>
#include <tinylogger.h>
#include <vector>
#include <thread>
#include <mutex>
/*
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <glm/gtx/vector_angle.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include <glm/ext/matrix_float4x4_precision.hpp>
*/
#include "glmIncludes.h"
#include <vulkan/vulkan.h>
#include <PxPhysicsAPI.h>





struct Model
{
	std::string MODEL_PATH;
	std::string TEXTURE_PATH;
	std::string NORMAL_PATH;
	std::string UUID = std::string("");
	std::string baseDir = "res/";
	glm::mat4 transform;
	//Model(std::string MODEL_PATH, std::string TEXTURE_PATH);
	std::vector<Vertex> vertices;
	std::vector<uint32_t> indices;
	
	union {
		physx::PxRigidStatic* rigidStatic;
		physx::PxRigidDynamic* rigidDynamic;
	} colision;

	glm::vec3 scaleVec = glm::vec3(1.0f);
	glm::vec3 translationVec = glm::vec3(0.0);
	glm::vec3 rotationVec = glm::vec3(0.0f);

	int pipelineIndex = 0;

	Material material;
	unsigned int statsFaces = 0;
	int isStatic = 0;
	VkBuffer vertexBuffer;
	VkDeviceMemory vertexBufferMemory;
	VkBuffer indexBuffer;
	VkDeviceMemory indexBufferMemory;
	VkImage textureImage;
	VkDeviceMemory textureImageMemory;
	VkImageView textureImageView;
	VkSampler textureSampler;
	VkImage normalImage;
	VkDeviceMemory normalImageMemory;
	VkImageView normalImageView;
	VkSampler normalSampler;
	
	uint32_t mipLevels;
	std::vector<VkDescriptorSet> descriptorSets;
	VkDescriptorSetLayout descriptorSetLayout;
	VkDescriptorPool descriptorPool;
	std::vector<VkBuffer> uniformBuffers;
	std::vector<VkDeviceMemory> uniformBuffersMemory;
	std::vector<void*> uniformBuffersMapped;

};

void init_model(Model* cModel, std::string MODEL_PATH, std::string TEXTURE_PATH);

void cleanup_model(VkDevice* device, Model* cModel);

void load_model(Model* cModel);

void init_model_resources(VkDevice* device, VkPhysicalDevice* physicalDevice, VkCommandPool commandPool, VkQueue graphicsQueue, Model* cModel, it_ImageResource* depthRes, std::vector<VkBuffer>* lightBuffers);
void mt_init_model_resources(VkDevice* device, VkPhysicalDevice* physicalDevice, VkCommandPool commandPool, VkQueue graphicsQueue, Model* cModel, it_ImageResource* depthRes, std::vector<VkBuffer>* lightBuffers, std::mutex& queueMutex);

void draw_model(Model* cModel, VkCommandBuffer commandBuffer, VkPipelineLayout graphicsPipelineLayout, int currentFrame);

