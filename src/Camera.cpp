#include <Camera.h>


Camera::Camera(float Winwidth, float Winheight)
{
	width = Winwidth;
	height = Winheight;
}

void Camera::UpdateMatrices(float FOV, Model* Model) // do not touch ever again for the love of god and everything that you love YOU HAVE SPENT HOURS FOR SOME FUCKING REASON SRT DOES NOT WORK BUT STR WORKS SO STFU
{
	model = glm::mat4(1.0f);
	glm::mat4 scaleMat = glm::scale(Model->scaleVec);
	glm::mat4 translateMat = glm::translate(glm::mat4(1.0f), Model->translationVec);
	glm::mat4 rotMat = glm::mat4(1.0f);
	rotMat *= glm::rotate(glm::mat4(1.0f), Model->rotationVec.z, glm::vec3(0.0f, 0.0f, 1.0f));
	rotMat *= glm::rotate(glm::mat4(1.0f), Model->rotationVec.y, glm::vec3(0.0f, 1.0f, 0.0f));
	rotMat *= glm::rotate(glm::mat4(1.0f), Model->rotationVec.x, glm::vec3(1.0f, 0.0f, 0.0f));
	model *= translateMat * rotMat * scaleMat;
	Model->transform = model;
	lightMat = glm::translate(lightPos);
	view = glm::lookAt(Position, Position + Orientation, Up); // world space
	proj = glm::perspective(glm::radians(FOV), width / (float)height, 0.1f, 1000.0f); // no idea
}

void Camera::UpdateInputs(GLFWwindow* window)
{
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
	{
		glfwSetWindowShouldClose(window, true);
	}
	if (glfwGetKey(window, GLFW_KEY_K) == GLFW_PRESS && !LockKey)
	{
		LockCamera = !LockCamera;
		firstClick = true;
		LockKey = true;
	}
	if (glfwGetKey(window, GLFW_KEY_K) == GLFW_RELEASE)
	{
		LockKey = false;
		firstClick = false;
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
			pitch = 89.5f;
			glfwSetCursorPos(window, (width / 2), (height / 2));
			return;
		}
		if (pitch < -89.5f)
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
		Position += speed *  glm::normalize(Orientation);
	}
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
	{
		Position += speed * -glm::normalize(Orientation);
	}
	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
	{
		Position += speed * -glm::normalize(glm::cross(Orientation, Up));
	}
	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
	{
		Position += speed *  glm::normalize(glm::cross(Orientation, Up));
	}
	if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
	{
		Position += speed * Up;
	}
	if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS)
	{
		Position += speed * -Up;
	}
	if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
	{
		speed = 0.004f;
	}
	if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_RELEASE)
	{
		speed = 0.001f;
	}
	

}