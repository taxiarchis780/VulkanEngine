#include "Surface.h"
#include <stdexcept>

void create_surface(VkSurfaceKHR* surface, VkInstance* instance, GLFWwindow* window)
{
    if (glfwCreateWindowSurface(*instance, window, nullptr, surface) != VK_SUCCESS)
    {
        throw std::runtime_error("ERROR: failed to create window surface!");
    }
}