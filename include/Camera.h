#ifndef __CAMERA_CLASS__
#define __CAMERA_CLASS__
#define VK_USE_PLATFORM_WIN32_KHR
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>
#include <vulkan/vulkan.h>
#include <functional>
/*
#define GLM_FORCE_RADIANS
#define GLM_ENABLE_EXPERIMENTAL
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <glm/gtx/vector_angle.hpp>
*/
#include "glmIncludes.h"
#include <chrono>
#include <Model.h>
#include <ResourceBuffer.h>

class Camera {
public:
	float width, height;
	glm::mat4 model;
	glm::mat4 view;
	glm::mat4 proj;
	
	glm::vec3 Position;// = glm::vec3(6.1f, 0.1f, 6.12f);
	glm::vec3 Orientation;// = glm::vec3(-2.5f, 0.0f, -0.5f);
	glm::vec3 Up = glm::vec3(0.0f, 0.0f, 1.0f);
	glm::vec3 lightPos = glm::vec3(0.0f, 0.0f, 0.0f);
	glm::vec3 lightColor = glm::vec3(0.86275f, 0.70588f, 0.47059f);
	glm::vec3 lightRot = glm::vec3(0.0f, 0.0f, 0.0f);
	GLFWmonitor* monitor;
	glm::mat4 lightMat;

	float velocity = 0.1f;
	float sensitivity = 100.0f;
	float pitch = 0.0f;
	float FOV = 90.0f;
	bool LockCamera = true; 
	bool firstClick = true;

	glm::vec3 offset = glm::vec3(10.0f);
	
	Camera(float width, float height);

	void UpdateMatrices(Model* model);
	int  pickModel(std::vector<Model*> scene, GLFWwindow* window);
	void UpdateInputs(GLFWwindow* window, std::function<void(GLFWwindow*, Camera*)> cameraFunction = 0);
};

void update_model_uniform_buffers(Model* cModel, Camera* camera, uint32_t currentImage);


#endif