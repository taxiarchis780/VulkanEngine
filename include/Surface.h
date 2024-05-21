#ifndef __SURFACE_H__
#define __SURFACE_H__

#include <vulkan/vulkan.h>
#include "GLFW/glfw3.h"
#include "GLFW/glfw3native.h"

struct it_Surface {
	VkSurfaceKHR surface;
};

void create_surface(VkSurfaceKHR* surface, VkInstance* instance, GLFWwindow* window);

#endif