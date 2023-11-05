#ifndef __ENTITY_CLASS_H__
#define __ENTITY_CLASS_H__
#include <Model.h>



class Entity
{
public:
	Entity();
	
	~Entity();
private:
	Model* model;
};


#endif
