#ifndef __CAMERA_CLASS__
#define __CAMERA_CLASS__
#define VK_USE_PLATFORM_WIN32_KHR
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>
#include <vulkan/vulkan.h>
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <glm/gtx/vector_angle.hpp>
#include <chrono>
#include <Model.h>

struct Camera {

	float width, height;
	glm::mat4 model;
	glm::mat4 view;
	glm::mat4 proj;


	glm::vec3 Position = glm::vec3(6.1f, 0.1f, 6.12f);
	glm::vec3 Orientation = glm::vec3(-2.5f, 0.0f, -0.5f);
	glm::vec3 Up = glm::vec3(0.0f, 0.0f, 1.0f);
	glm::vec3 lightPos = glm::vec3(0.0f, 0.0f, 0.0f);
	glm::vec3 lightColor = glm::vec3(0.86275f, 0.70588f, 0.47059f);
	GLFWmonitor* monitor;
	glm::mat4 lightMat;

	float speed = 0.001f;
	float sensitivity = 100.0f;
	float pitch = 0.0f;
	bool LockCamera = true; 
	bool firstClick = true;
	bool LockKey = false;

	Camera(float width, float height);

	void UpdateMatrices(float FOV, Model* model);
	void UpdateInputs(GLFWwindow* window);
};



#endif