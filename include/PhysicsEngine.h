#ifndef __PHYSICS_ENGINE_CLASS__
#define __PHYSICS_ENGINE_CLASS__

#include <Model.h>
#include <PxPhysics.h>
#include <PxPhysicsAPI.h>
#include <util.h>
#include <stdexcept>
using namespace physx;

class PhysicsEngine
{
public:
	PhysicsEngine();
	~PhysicsEngine();

	void addPhysicsObj(Model* cModel, bool isStatic, physx::PxMat44 mat);
	void applyForceToRigidBody(Model* cModel);
	void Update();
private:
	
	inline static PxDefaultAllocator			gAllocator;
	inline static PxDefaultErrorCallback		gErrorCallback;
	inline static PxFoundation*					gFoundation;
	inline static PxPhysics*					gPhysics;
	inline static PxDefaultCpuDispatcher*		gDispatcher;
	inline static PxScene*						gScene;
	inline static PxMaterial*					gMaterial;
	inline static PxPvd*						gPvd;
	inline static bool							gIsRunning;
	
	
};

#endif
