#include "World.h"



void update_light_uniform_buffers(it_lightBufferResource* lightRes, Camera* camera, uint32_t currentImage)
{
	
	LightsUniformBufferObject lubo{};
	lubo.lightPos = camera->lightPos;
	lubo.lightRot = camera->lightRot;
	lubo.lightColor = camera->lightColor;

	memcpy(lightRes->lightBuffersMapped[currentImage], &lubo, sizeof(lubo));
}