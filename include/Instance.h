#ifndef __INSTANCE_H__
#define __INSTANCE_H__

#include <vulkan/vulkan.h>
#include <vector>

/* PLEASE MOVE TO SOMEWHERE ELSE FROM validationLayers till checkValidationLayerSupport FFS */
extern const std::vector<const char*> validationLayers;

extern bool enableValidationLayers;

struct it_Intance
{
	VkInstance instance;
};

void create_instance(VkInstance* instance);

#endif