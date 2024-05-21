#include <Camera.h>


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
	lightMat = glm::translate(lightPos);
	view = glm::lookAt(Position, Position + Orientation, Up); 
	proj = glm::perspective(glm::radians(FOV), width / (float)height, 0.1f, 10000.0f);

}


int Camera::pickModel(std::vector<Model*> scene, GLFWwindow* window)
{
	
	for (int i = 0; i < scene.size(); ++i)
	{
		glm::vec4 clipSpacePos = proj * (view * glm::vec4(scene[i]->translationVec, 1.0));
		glm::vec3 ndcSpacePos;
		if (!clipSpacePos.w)
			printf("Panik!");
		ndcSpacePos.x = clipSpacePos.x / clipSpacePos.w;
		ndcSpacePos.y = clipSpacePos.y / clipSpacePos.w;
		ndcSpacePos.z = clipSpacePos.z / clipSpacePos.w;
		//printf("\rclipSpacePos.x %.3f,clipSpacePos.y: %.3f,clipSpacePos.z: %.3f", clipSpacePos.x, clipSpacePos.y, clipSpacePos.z);
		glm::vec2 windowSpacePos;
		windowSpacePos.x = ((ndcSpacePos.x + 1.0) / 2.0); //* viewSize + viewOffset;
		windowSpacePos.y = ((ndcSpacePos.y + 1.0) / 2.0);
		
		double xClick, yClick;
		glfwGetCursorPos(window, &xClick, &yClick);
		yClick = height - yClick;
		float nonNormalizedValueX = windowSpacePos.x * (width);
		float nonNormalizedValueY = windowSpacePos.y * height;
		
		//printf("nonNormalizedValueX: %f\n", nonNormalizedValueX);
		//printf("nonNormalizedValueY: %f\n", nonNormalizedValueY);
		//printf("xClick: %f yClick: %f\n", xClick, yClick);
		
		if (xClick > nonNormalizedValueX - 50.0 && xClick < nonNormalizedValueX + 50.0)
		{
			
			if (yClick > nonNormalizedValueY - 50.0 && yClick < nonNormalizedValueY + 50.0)
			{
				
				return i;
			}
		}
		
	}
	return -1;
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

