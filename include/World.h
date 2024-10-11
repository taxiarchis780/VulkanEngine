#ifndef __WORLD_H__
#define __WORLD_H__

#include <vulkan/vulkan.h>
#include "Camera.h"

void update_light_uniform_buffers(it_lightBufferResource* lightRes, Camera* camera, uint32_t currentImage);

#endif