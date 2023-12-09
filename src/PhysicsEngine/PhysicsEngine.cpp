#include <PhysicsEngine.h>


PhysicsEngine::PhysicsEngine() // ditch VCPKG start linking by yourself
{
	gFoundation = NULL;
	gPhysics = NULL;
	gDispatcher = NULL;
	gScene = NULL;
	gMaterial = NULL;
	gPvd = NULL;
	gIsRunning = false;


	//init physics
	gFoundation = PxCreateFoundation(PX_PHYSICS_VERSION, gAllocator, gErrorCallback);

	gPvd = PxCreatePvd(*gFoundation);
	printf("Pointer : 0x%p\n", gPvd);
	PxPvdTransport* transport = PxDefaultPvdSocketTransportCreate("127.0.01", 5425, 10);
	gPvd->connect(*transport, PxPvdInstrumentationFlag::eALL);

	gPhysics = PxCreatePhysics(PX_PHYSICS_VERSION, *gFoundation, PxTolerancesScale(), true, gPvd);

	gMaterial = gPhysics->createMaterial(.5f, .5f, .25f);
	
	// init scene
	PxCudaContextManager* cudaContextManager = NULL;
	if (PxGetSuggestedCudaDeviceOrdinal(gFoundation->getErrorCallback()) >= 0)
	{
		PxCudaContextManagerDesc cudaContextManagerDesc;
		cudaContextManager = PxCreateCudaContextManager(*gFoundation, cudaContextManagerDesc, PxGetProfilerCallback());
		if (cudaContextManager && !cudaContextManager->contextIsValid())
		{
			cudaContextManager->release();
			cudaContextManager = NULL;
		}
	}
	if (cudaContextManager == NULL)
	{
		PxGetFoundation().error(PxErrorCode::eINVALID_OPERATION, PX_FL, "Failed to initialize CUDA!\n");
	}

	PxSceneDesc sceneDesc(gPhysics->getTolerancesScale());
	sceneDesc.gravity = PxVec3(0.0f, -9.81f, 0.0f);
	const PxU32 numCores = PxThread::getNbPhysicalCores();
	gDispatcher = PxDefaultCpuDispatcherCreate(numCores == 0 ? 0 : numCores - 1);
	printf("Number of PhysX Cores: %d\n", numCores);
	sceneDesc.cpuDispatcher = gDispatcher;
	sceneDesc.filterShader = PxDefaultSimulationFilterShader;
	sceneDesc.cudaContextManager = cudaContextManager;
	sceneDesc.staticStructure = PxPruningStructureType::eDYNAMIC_AABB_TREE;
	sceneDesc.flags |= PxSceneFlag::eENABLE_PCM;
	sceneDesc.flags |= PxSceneFlag::eENABLE_GPU_DYNAMICS;
	sceneDesc.broadPhaseType = PxBroadPhaseType::eGPU;
	sceneDesc.solverType = PxSolverType::eTGS;
	gScene = gPhysics->createScene(sceneDesc);


	PxPvdSceneClient* pvdClient = gScene->getScenePvdClient();
	pvdClient->setScenePvdFlag(physx::PxPvdSceneFlag::eTRANSMIT_CONSTRAINTS, true);
	pvdClient->setScenePvdFlag(physx::PxPvdSceneFlag::eTRANSMIT_CONTACTS, true);
	pvdClient->setScenePvdFlag(physx::PxPvdSceneFlag::eTRANSMIT_SCENEQUERIES, true);
	physx::PxTransform transform(physx::PxVec3(0.0f, 0.0f, 0.0f));
	PxRigidStatic* groundPlane = PxCreatePlane(*gPhysics, PxPlane(0, 1, 0, 1), *gMaterial);
	//PxRigidStatic* groundPlane = physx::PxCreateStatic(*gPhysics,  transform, physx::PxBoxGeometry(1000.0f, 1.0f, 1000.0f), *gMaterial);
	gScene->addActor(*groundPlane);
	PxRigidDynamic* body1 = NULL;
	float halfExtent = .5f;
	PxShape* shape = gPhysics->createShape(PxBoxGeometry(halfExtent, halfExtent, halfExtent), *gMaterial);
	PxU32 size = 10;
	PxTransform t(PxVec3(0));
	for (PxU32 i = 0; i < size; i++) {
		for (PxU32 j = 0; j < size - i; j++) {
			PxTransform localTm(PxVec3(PxReal(j * 2) - PxReal(size - i), PxReal(i * 2 + 1), 0) * halfExtent);
			PxRigidDynamic* body = gPhysics->createRigidDynamic(t.transform(localTm));
			body->attachShape(*shape);
			PxRigidBodyExt::updateMassAndInertia(*body, 10.0f);
			body->setRigidBodyFlag(PxRigidBodyFlag::eENABLE_SPECULATIVE_CCD, true);
			gScene->addActor(*body);
		}
	}

}



PhysicsEngine::~PhysicsEngine()
{
	
}



void PhysicsEngine::addPhysicsObj(Model* cModel, bool isStatic, physx::PxMat44 mat)
{
	//PxMat44 mat = glmMat4ToPhysxMat4(transform);
	//PxTransform Ptransform(PxVec3(1.0f, 1.0f, 1.0f), PxQuat(1.0f));
	PxTransform Ptransform(mat);


	if (isStatic)
	{
		cModel->colision.rigidStatic = physx::PxCreateStatic(*gPhysics, Ptransform, physx::PxBoxGeometry(1.0f, 1.0f, 1.0f), *gMaterial);
		cModel->colision.rigidStatic = gPhysics->createRigidStatic(Ptransform);
		physx::PxShape* shape = gPhysics->createShape(physx::PxBoxGeometry(1.0f, 1.0f, 1.0f), *gMaterial);
		cModel->colision.rigidStatic->attachShape(*shape);
		gScene->addActor(*cModel->colision.rigidStatic);
	}
	else {
		cModel->colision.rigidDynamic = gPhysics->createRigidDynamic(Ptransform);
		physx::PxShape* shape = gPhysics->createShape(physx::PxBoxGeometry(1.0f, 1.0f, 1.0f), *gMaterial);
		cModel->colision.rigidDynamic->attachShape(*shape);
		cModel->colision.rigidDynamic->setAngularDamping(.1f);
		cModel->colision.rigidDynamic->setLinearDamping(.1f);
		cModel->colision.rigidDynamic->setSleepThreshold(0.0f);
		cModel->colision.rigidDynamic->setWakeCounter(100.0f);
		cModel->colision.rigidDynamic->setRigidBodyFlag(PxRigidBodyFlag::eENABLE_SPECULATIVE_CCD, true);
		gScene->addActor(*cModel->colision.rigidDynamic);
	}

	return;
}

void PhysicsEngine::applyForceToRigidBody(Model* cModel) {
	
	physx::PxRigidBody* myRigidBody = cModel->colision.rigidDynamic->is<physx::PxRigidBody>();
	myRigidBody->addForce(physx::PxVec3(0.0f, 1000.0f, 0.0f));
}

void PhysicsEngine::Update()
{
	
	gScene->simulate(1.0f / 60.0f);

	gScene->fetchResults(true); 
}