#ifndef __ENTITY_CLASS__
#define __ENTITY_CLASS__
#include <Model.h>



class Entity
{
public:	
	Entity();
	~Entity();

	glm::vec3 scaleVec = glm::vec3(1.0f);
	glm::vec3 translationVec = glm::vec3(0.0f);
	glm::vec3 rotationVec = glm::vec3(0.0f);
	
	
private:
};


#endif
