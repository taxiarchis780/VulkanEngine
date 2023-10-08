#ifndef __MODEL_CLASS__
#define __MODEL_CLASS__
#include <Vertex.h>
#include <string>
#include <tinylogger.h>
#include <vector>
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <glm/gtx/vector_angle.hpp>
#include <BasicCollider.h>

class Model // FOR SOME FUCKING REASON THE ORDER THAT I DECLARE SCALE TRANSLATION AND ROTATION MATTERS AND FOR SOME OTHER FUCKING REASON THIS CANT BE STRUCT BECAUSE IT SEG FAULTS WTF IS WRONG WITH THIS CURSED FILE
{
public:
	std::string MODEL_PATH;
	std::string TEXTURE_PATH;
	std::string UUID = std::string("");
	std::string baseDir = "res/";
	glm::mat4 transform;
	Model(std::string MODEL_PATH, std::string TEXTTURE_PATH);
	std::vector<Vertex> vertices;
	std::vector<uint32_t> indices;
	glm::vec3 scaleVec = glm::vec3(1.0f);


	glm::vec3 translationVec = glm::vec3(0.0);
	glm::vec3 rotationVec = glm::vec3(0.0f);

	BoundingBox collider;

	Material material;
	unsigned int statsFaces = 0;
	VkBuffer vertexBuffer;
	VkDeviceMemory vertexBufferMemory;
	VkBuffer indexBuffer;
	VkDeviceMemory indexBufferMemory;
	VkImage textureImage;
	VkDeviceMemory textureImageMemory;
	VkImageView textureImageView;
	VkSampler textureSampler;
	
	uint32_t mipLevels;
	std::vector<VkDescriptorSet> descriptorSets;
	VkDescriptorPool descriptorPool;
	std::vector<VkBuffer> uniformBuffers;
	std::vector<VkDeviceMemory> uniformBuffersMemory;
	std::vector<void*> uniformBuffersMapped;
};


#endif