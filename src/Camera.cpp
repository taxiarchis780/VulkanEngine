#include <Camera.h>
#include "util.h"

Camera::Camera(float Winwidth, float Winheight)
{
	width = Winwidth;
	height = Winheight;
}

void Camera::UpdateMatrices(Model* Model) 
{
	//Model->Update();
	model = glm::mat4(1.0f);
	glm::mat4 scaleMat = glm::scale(Model->scaleVec);
	/*
	if (Model->UUID == "Laocoon" && Model->translationVec.z >= 0.0f)
	{
		//Model->translationVec.z -= 0.001;
		
		glm::quat quaternion = glm::quatLookAt(Model->rotationVec/glm::length(Model->rotationVec), Up);
		printf("\r X: %.1f, Y: %.1f, Z: %.1f, W: %.1f", quaternion.x, quaternion.y, quaternion.z, quaternion.w);
	}
	*/

	glm::mat4 translateMat = glm::translate(glm::mat4(1.0f), Model->translationVec);
	glm::mat4 rotMat = glm::mat4(1.0f);

	rotMat *= glm::rotate(glm::mat4(1.0f), Model->rotationVec.z, glm::vec3(0.0f, 0.0f, 1.0f));
	rotMat *= glm::rotate(glm::mat4(1.0f), Model->rotationVec.y, glm::vec3(0.0f, 1.0f, 0.0f));
	rotMat *= glm::rotate(glm::mat4(1.0f), Model->rotationVec.x, glm::vec3(1.0f, 0.0f, 0.0f));
	model *= translateMat * rotMat * scaleMat;
	Model->transform = model;
	glm::mat4 lightRotMat = glm::mat4(1.0f);

	lightRotMat *= glm::rotate(glm::mat4(1.0f), lightRot.z, glm::vec3(0.0f, 0.0f, 1.0f));
	lightRotMat *= glm::rotate(glm::mat4(1.0f), lightRot.y, glm::vec3(0.0f, 1.0f, 0.0f));
	lightRotMat *= glm::rotate(glm::mat4(1.0f), lightRot.x, glm::vec3(1.0f, 0.0f, 0.0f));

	lightMat = glm::translate(lightPos) * lightRotMat;
	view = glm::lookAt(Position, Position + Orientation, Up); 
	proj = glm::perspective(glm::radians(FOV), width / (float)height, 0.1f, 10000.0f);

}





int Camera::pickModel(std::vector<Model*> scene, GLFWwindow* window)
{
	
	std::vector<int> insideModels; // List to store indices of models where click is inside

	// Variables to track closest model
	int closestModelIndex = -1;
	float closestDistance = std::numeric_limits<float>::max();

	for (int i = 0; i < scene.size(); ++i)
	{
		const Model* cModel = scene[i];
		if (strcmp(cModel->UUID.c_str(), "skybox") == 0)
			continue;

		// finding out the extreme points
		glm::vec2 UpScreen = glm::vec2(std::numeric_limits<float>::max(), std::numeric_limits<float>::max());
		glm::vec2 DownScreen = glm::vec2(std::numeric_limits<float>::lowest(), std::numeric_limits<float>::lowest());
		glm::vec2 RightScreen = glm::vec2(std::numeric_limits<float>::lowest(), std::numeric_limits<float>::lowest());
		glm::vec2 LeftScreen = glm::vec2(std::numeric_limits<float>::max(), std::numeric_limits<float>::max());

		// Iterate through each vertex in the current model
		for (const Vertex& vertex : cModel->vertices) {
			// Calculate world position of the vertex
			glm::vec3 scaledPos = vertex.pos * cModel->scaleVec;
			glm::vec3 rotatedPos = glm::mat3(cModel->transform) * vertex.pos;
			glm::vec3 worldPos = rotatedPos + cModel->translationVec;

			// Convert world position to screen space
			glm::vec2 screenPos = util::worldToScreen(worldPos, proj, view);

			// Update the extrema in screen space
			if (screenPos.y < UpScreen.y) {
				UpScreen = screenPos;
			}
			if (screenPos.y > DownScreen.y) {
				DownScreen = screenPos;
			}
			if (screenPos.x > RightScreen.x) {
				RightScreen = screenPos;
			}
			if (screenPos.x < LeftScreen.x) {
				LeftScreen = screenPos;
			}
		}

		//if (UpScreen.x < 0.0f)
		//	UpScreen.x = 0.0f;
		//if (UpScreen.y < 0.0f)
		//	UpScreen.y = 0.0f;
		//if (DownScreen.x < 0.0f)
		//	DownScreen.x = 0.0f;
		//if (DownScreen.y < 0.0f)
		//	DownScreen.y = height;
		//if (RightScreen.x < 0.0f)
		//	RightScreen.x = width;
		//if (RightScreen.y < 0.0f)
		//	RightScreen.y = 0.0f;
		//if (LeftScreen.x < 0.0f)
		//	LeftScreen.x = 0.0f;
		//if (LeftScreen.y < 0.0f)
		//	LeftScreen.y = 0.0f;

		glm::vec2 click = glm::vec2(0.64f, 0.64f);
		double xpos, ypos = 0.0;

		glfwGetCursorPos(window, &xpos, &ypos);
		click.x = xpos / width;
		click.y = ((ypos) / height);
		bool inside = util::isInsideQuadrilateral(click, UpScreen, RightScreen, DownScreen, LeftScreen);
		if (inside) {
			// Calculate distance from camera to the model's origin (you may adjust this distance calculation as needed)
			glm::vec3 modelOrigin = cModel->translationVec; // Assuming origin is at the model's translation vector

			// Calculate distance from camera (you need to implement or use your existing depth calculation logic here)
			float distanceToClick = glm::distance(util::worldToScreen(cModel->translationVec, proj, view), click);
			//printf("Kite points in screen space:\n");
			//printf("Up    (%.2f, %.2f)\n", UpScreen.x, UpScreen.y);
			//printf("Right (%.2f, %.2f)\n", RightScreen.x, RightScreen.y);
			//printf("Down  (%.2f, %.2f)\n", DownScreen.x, DownScreen.y);
			//printf("Left  (%.2f, %.2f)\n", LeftScreen.x, LeftScreen.y);
			//printf("\rPoint (%.2f, %.2f) is inside the %s.\n", click.x, click.y, cModel->UUID.c_str());


			// Check if this model is closer to the camera than the current closest one
			if (distanceToClick < closestDistance) {
				closestDistance = distanceToClick;
				closestModelIndex = i;
			}

			// Optionally, you can store all models that are inside the quadrilateral
			insideModels.push_back(i);
		}
	}

	// Return the index of the closest model (you can change this as per your requirement)
	return closestModelIndex;
}


void Camera::UpdateInputs(GLFWwindow* window, std::function<void(GLFWwindow*,Camera*)> cameraFunction)
{
	if (cameraFunction)
	{
		cameraFunction(window, this);
		return;
	}
	
	if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) != GLFW_PRESS || glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS)
	{
		
		if (LockCamera)
		{
			glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);

			return;
		}
		else {
			glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
			if (firstClick)
				glfwSetCursorPos(window, width / 2, height / 2);
		}
		glm::vec3 oldOrientation = glm::vec3(Orientation.x, Orientation.y, Orientation.z-0.05f);
		double mouseX;
		double mouseY;
		glfwGetCursorPos(window, &mouseX, &mouseY);

		float rotX = sensitivity * (float)(glm::floor((width / 2)) - mouseX) / width;
		float rotY = sensitivity * (float)(glm::floor((height / 2)) - mouseY) / height;
		pitch += rotY;
		if (pitch > 89.5f)
		{
			pitch = 89.0f;
			glfwSetCursorPos(window, (width / 2), (height / 2));
			return;
		}
		if (pitch < -89.0f)
		{
			pitch = -89.5f;
			glfwSetCursorPos(window, (width / 2), (height / 2));
			return;
		}

		glm::vec3 newOrientation = glm::rotate(Orientation, glm::radians(rotY), glm::normalize(glm::cross(Orientation, Up)));


		Orientation = glm::rotate(newOrientation, glm::radians(rotX), Up); 

		glfwSetCursorPos(window, (width / 2), (height / 2));
	}

	
	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
	{
		Position += velocity *  glm::normalize(Orientation);
	}
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
	{
		Position += velocity * -glm::normalize(Orientation);
	}
	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
	{
		Position += velocity * -glm::normalize(glm::cross(Orientation, Up));
	}
	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
	{
		Position += velocity *  glm::normalize(glm::cross(Orientation, Up));
	}
	if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
	{
		Position += velocity * Up;
	}
	if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS)
	{
		Position += velocity* -Up;
	}
	if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
	{
		velocity = 0.1f * 4;
	}
	if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_RELEASE)
	{
		velocity = 0.1f;
	}
	

}



glm::mat4 calculate_light_space_matrix_dLight(glm::vec3 lightPosition, glm::vec3 lightDirection)
{
	// Compute light direction (assuming it's a directional light for simplicity)

	// Compute light view matrix
	glm::mat4 lightViewMatrix = glm::lookAt(-lightDirection, glm::vec3(0.0f), glm::vec3(0.0f, 0.0f, 1.0f));

	// Compute light projection matrix (orthographic projection example)
	float orthoSize = 10.0f;
	float nearPlane = 1.0f;
	float farPlane = 50.0f;
	glm::mat4 lightProjectionMatrix = glm::ortho(-orthoSize, orthoSize, -orthoSize, orthoSize, nearPlane, farPlane);

	// Compute light space matrix
	return lightProjectionMatrix * lightViewMatrix;
}


void update_model_uniform_buffers(Model* cModel, Camera* camera, uint32_t currentImage)
{
	static auto startTime = std::chrono::high_resolution_clock::now();

	auto currentTime = std::chrono::high_resolution_clock::now();
	float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

	camera->UpdateMatrices(cModel);

	UniformBufferObject ubo{};
	ubo.transform = cModel->transform;
	ubo.view = camera->view;
	ubo.proj = camera->proj;
	glm::vec3 lightPos = glm::vec3(10.3f, 27.4f, 3.2f);

	glm::vec3 lightTarget = glm::vec3(0.05f, 22.5f, -0.432f);
	glm::vec3 lightUp = glm::vec3(0.0f, 1.0f, 0.0f);

	// Calculate the light's view matrix
	glm::mat4 lightView = glm::lookAt(glm::vec3(20.3f, 27.4f, 23.2f), lightTarget, camera->Up);

	// Calculate the light's orthographic projection matrix
	glm::mat4 lightProjection = glm::ortho<float>(-50, 50, -50, 50, 0, 100);;
	lightProjection[1][1] *= -1;
	// Calculate the light space matrix
	glm::mat4 lightSpaceMatrix = lightProjection * lightView;
	glm::mat4 biasMatrix(
		0.5, 0.0, 0.0, 0.0,
		0.0, 0.5, 0.0, 0.0,
		0.0, 0.0, 0.5, 0.0,
		0.5, 0.5, 0.5, 1.0
	);
	ubo.lightSpaceMat = lightSpaceMatrix * biasMatrix;
	ubo.cameraPos = camera->Position;
	ubo.material = cModel->material;
	ubo.time = time;
	ubo.proj[1][1] *= -1;

	memcpy(cModel->uniformBuffersMapped[currentImage], &ubo, sizeof(ubo));
}


