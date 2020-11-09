#include "Components/PhysicsComponent.h"
#include "Components/LocationComponent.h"
#include "Components/Coordinator.h"
#include "Debugging/Assert.h"
#include "Time/GameClock.h"
#include "Debugging/DevConfiguration.h"


//#pragma warning( push )
//    #pragma warning( disable : 4127 )  // conditional expression is constant
//    #pragma warning( disable : 4099 )  // '': type name first seen using 'class' now seen using 'struct'
//    #include "btBulletDynamicsCommon.h" 
//#pragma warning( pop )

using namespace yaget;
using namespace DirectX;

#if 0
namespace yaget
{
    namespace conv
    {
        btTransform Matrix(const SimpleMath::Matrix& matrix)
        {
            SimpleMath::Vector3 scale;
            SimpleMath::Quaternion rotation;
            SimpleMath::Vector3 translation;
            SimpleMath::Matrix workMatrix(matrix);
            if (workMatrix.Decompose(scale, rotation, translation))
            {
                btTransform trans(btQuaternion(rotation.x, rotation.y, rotation.z, rotation.w));
                trans.setOrigin(btVector3(translation.x, translation.y, translation.z));
                return trans;
            }
    
            YAGET_ASSERT(false, "Could not decompose matrix.");
            return btTransform::getIdentity();
        }

        btTransform Matrix(const math3d::Vector3& position, const math3d::Quaternion& orientation)
        {
            return Matrix(math3d::Matrix::CreateFromQuaternion(orientation) * math3d::Matrix::CreateTranslation(position));
        }
    
        SimpleMath::Matrix Matrix(const btTransform& trans)
        {
            btTransformFloatData floatData;
            trans.serializeFloat(floatData);
            SimpleMath::Matrix retMatrix(SimpleMath::Vector4(floatData.m_basis.m_el[0].m_floats),
                SimpleMath::Vector4(floatData.m_basis.m_el[1].m_floats),
                SimpleMath::Vector4(floatData.m_basis.m_el[2].m_floats),
                SimpleMath::Vector4(floatData.m_origin.m_floats));
    
            return retMatrix;
        }
    } // namespace conv
} // namespace yaget


namespace
{
    class YagetDefaultMotionState : public btDefaultMotionState
    {
    public:
        BT_DECLARE_ALIGNED_ALLOCATOR();

        YagetDefaultMotionState(comp::PhysicsComponent::Params::WorldTransformCallback_t worldTransformCallback,
            const btTransform& startTrans = btTransform::getIdentity(), 
            const btTransform& centerOfMassOffset = btTransform::getIdentity())
            : btDefaultMotionState(startTrans, centerOfMassOffset)
            , mWorldTransformCallback(worldTransformCallback)
        {
            TrigerCallback(startTrans);
        }

        /////synchronizes world transform from user to physics
        //virtual void getWorldTransform(btTransform& centerOfMassWorldTrans) const
        //{
        //    btDefaultMotionState::getWorldTransform(centerOfMassWorldTrans);
        //}

        ///synchronizes world transform from physics to user
        ///Bullet only calls the update of worldtransform for active objects
        virtual void setWorldTransform(const btTransform& centerOfMassWorldTrans)
        {
            btDefaultMotionState::setWorldTransform(centerOfMassWorldTrans);
            btTransform bodyMatrix;
            getWorldTransform(bodyMatrix);
            TrigerCallback(bodyMatrix);
        }

    private:
        void TrigerCallback(const btTransform& currentMatrix)
        {
            if (mWorldTransformCallback)
            {
                math3d::Vector3 scale;
                math3d::Quaternion quat;
                math3d::Vector3 pos;
                if (conv::Matrix(currentMatrix).Decompose(scale, quat, pos))
                {
                    mWorldTransformCallback(pos, quat);
                }
            }
        }

        comp::PhysicsComponent::Params::WorldTransformCallback_t mWorldTransformCallback;
    };
} // namespace
#endif // 0

yaget::comp::PhysicsWorldComponent::PhysicsWorldComponent(Id_t id)
    : Component(id)
    //, mCollisionConfiguration(std::make_unique<btDefaultCollisionConfiguration>())
    //, mDispatcher(std::make_unique<btCollisionDispatcher>(mCollisionConfiguration.get()))
    //, mOverlappingPairCache(std::make_unique<btDbvtBroadphase>())
    //, mSolver(std::make_unique<btSequentialImpulseConstraintSolver>())
    //, mDynamicsWorld(std::make_unique<btDiscreteDynamicsWorld>(mDispatcher.get(), mOverlappingPairCache.get(), mSolver.get(), mCollisionConfiguration.get()))
{

    //static physx::PxDefaultErrorCallback gDefaultErrorCallback;
    //static physx::PxDefaultAllocator gDefaultAllocatorCallback;

    //physx::PxFoundation* foundation = PxCreateFoundation(PX_PHYSICS_VERSION, gDefaultAllocatorCallback, gDefaultErrorCallback);
    //YAGET_ASSERT(foundation, "xCreateFoundation failed.");
}

void yaget::comp::PhysicsWorldComponent::Update(const time::GameClock& /*gameClock*/, yaget::metrics::Channel& /*channel*/)
{
    //float stepInSeconds = time::FromTo<float>(gameClock.GetDeltaTime(), time::kMicrosecondUnit, time::kSecondUnit);
    //mDynamicsWorld->stepSimulation(stepInSeconds);
}





comp::PhysicsComponent::PhysicsComponent(Id_t id, PhysicsWorldComponent* physicsWorld, const yaget::physics::CollisionShape& collisionSHape, comp::LocationComponent* locationComponent)
    : PhysicsComponent(id, physicsWorld, collisionSHape, locationComponent->GetPosition(), locationComponent->GetOrientation())
{
    YAGET_ASSERT(dev::CurrentThreadIds().IsThreadLogic(), "Creation of component must be called from LOGIC thread.");

    ConnectTrigger(PhysicsComponent::TransformChanged, [locationComponent](const comp::Component& from)
    {
        const PhysicsComponent& physicsComponent = static_cast<const PhysicsComponent&>(from);
        locationComponent->SetPosition(physicsComponent.GetPosition());
        locationComponent->SetOrientation(physicsComponent.GetOrientation());
    });
}

comp::PhysicsComponent::PhysicsComponent(Id_t id, PhysicsWorldComponent* /*physicsWorld*/, const yaget::physics::CollisionShape& /*collisionSHape*/, const math3d::Vector3& position, const math3d::Quaternion& orientation)
    : Component(id)
    //, mDynamicsWorld(physicsWorld->mDynamicsWorld.get())
    //, mColisionlShape(collisionSHape.GetShape())
    , mPosition(position)
    , mOrientation(orientation)
{
    AddSupportedTrigger(TransformChanged);

    //btTransform startTransform = conv::Matrix(mPosition, mOrientation);
    //btVector3 localInertia(0, 0, 0);

    ////rigidbody is dynamic if and only if mass is non zero, otherwise static
    //if (collisionSHape.GetMass() != 0.f)
    //{
    //    mColisionlShape->calculateLocalInertia(collisionSHape.GetMass(), localInertia);
    //}

    //auto motionCallback = [this](auto&&... params) { onSigTransformChanged(params...); };
    //btDefaultMotionState* motionState = new YagetDefaultMotionState(motionCallback, startTransform);
    //btRigidBody::btRigidBodyConstructionInfo rbInfo(collisionSHape.GetMass(), motionState, mColisionlShape, localInertia);
    //mRigidBody = new btRigidBody(rbInfo);

    //mDynamicsWorld->addRigidBody(mRigidBody);
}

comp::PhysicsComponent::PhysicsComponent(Id_t id, PhysicsWorldComponent* /*physicsWorld*/, const Params& /*params*/)
    : Component(id)
    //: PhysicsComponent(id, physicsWorld->mDynamicsWorld.get(), params)
{
    AddSupportedTrigger(TransformChanged);
    //mDynamicsWorld->addRigidBody(mRigidBody);
}

comp::PhysicsComponent::PhysicsComponent(Id_t id, btDiscreteDynamicsWorld* /*dynamicsWorld*/, const Params& /*params*/)
    : Component(id)
    //, mDynamicsWorld(dynamicsWorld)
    //, mColisionlShape(params.collisionShape)
{
    AddSupportedTrigger(TransformChanged);

    //YAGET_ASSERT(mDynamicsWorld, "Can not create Physics Component when dynamicsWorld is nullptr.");

    ///// Create Dynamic Objects
    //btTransform startTransform;
    //if (params.matrix)
    //{
    //    startTransform = conv::Matrix(*params.matrix);
    //}
    //else
    //{
    //    startTransform.setIdentity();
    //}

    //btScalar mass(params.mass);

    ////rigidbody is dynamic if and only if mass is non zero, otherwise static
    //bool isDynamic = (mass != 0.f);

    //btVector3 localInertia(0, 0, 0);
    //if (isDynamic)
    //{
    //    mColisionlShape->calculateLocalInertia(mass, localInertia);
    //}

    //auto callback = params.worldTransformCallback ? params.worldTransformCallback : std::bind(&PhysicsComponent::onSigTransformChanged, this, std::placeholders::_1, std::placeholders::_2);
    //btDefaultMotionState* motionState = new YagetDefaultMotionState(callback, startTransform);
    //btRigidBody::btRigidBodyConstructionInfo rbInfo(mass, motionState, mColisionlShape, localInertia);
    //mRigidBody = new btRigidBody(rbInfo);
    //mUpdateDynamicsWorld = true;
}

comp::PhysicsComponent::~PhysicsComponent()
{
    //delete mRigidBody->getMotionState();
    //mDynamicsWorld->removeCollisionObject(mRigidBody);
    //delete mRigidBody;
    //delete mColisionlShape;
    //mUpdateDynamicsWorld = false;
}

void comp::PhysicsComponent::Tick(const time::GameClock& /*gameClock*/)
{
    //if (mUpdateDynamicsWorld && mDynamicsWorld && mRigidBody)
    //{
    //    mUpdateDynamicsWorld = false;
    //    mDynamicsWorld->addRigidBody(mRigidBody);
    //}
}

void comp::PhysicsComponent::onSigTransformChanged(const math3d::Vector3& position, const math3d::Quaternion& orientation)
{
    mPosition = position;
    mOrientation = orientation;

    TriggerSignal(TransformChanged);
}

void comp::PhysicsComponent::SetMatrix(const SimpleMath::Matrix& /*matrix*/)
{
    //btTransform trans = conv::Matrix(matrix);
    //if (mRigidBody->getMotionState())
    //{
    //    mRigidBody->getMotionState()->setWorldTransform(trans);
    //}
    //else
    //{
    //    mRigidBody->setWorldTransform(trans);
    //}
}

SimpleMath::Matrix comp::PhysicsComponent::GetMatrix() const
{
    return {};
    //btTransform trans;
    //if (mRigidBody && mRigidBody->getMotionState())
    //{
    //    mRigidBody->getMotionState()->getWorldTransform(trans);
    //}
    //else
    //{
    //    trans = mRigidBody->getWorldTransform();
    //}

    //return conv::Matrix(trans);
}


//comp::PhysicsComponentPool::PhysicsComponentPool()
//    : ComponentPool<PhysicsComponent, 1000>()
//    //, mCollisionConfiguration(std::make_unique<btDefaultCollisionConfiguration>())
//    //, mDispatcher(std::make_unique<btCollisionDispatcher>(mCollisionConfiguration.get()))
//    //, mOverlappingPairCache(std::make_unique<btDbvtBroadphase>())
//    //, mSolver(std::make_unique<btSequentialImpulseConstraintSolver>())
//    //, mDynamicsWorld(std::make_unique<btDiscreteDynamicsWorld>(mDispatcher.get(), mOverlappingPairCache.get(), mSolver.get(), mCollisionConfiguration.get()))
//{
//    //mDynamicsWorld->setGravity(btVector3(0, -10, 0));
//}
//
//comp::PhysicsComponentPool::~PhysicsComponentPool()
//{
//    
//}
//
//void comp::PhysicsComponentPool::SetDebugDraw(btIDebugDraw* /*debugDraw*/)
//{
//    //mDebugDraw.reset(debugDraw);
//    //mDynamicsWorld->setDebugDrawer(mDebugDraw.get());
//}
//
//void comp::PhysicsComponentPool::Tick(const time::GameClock& gameClock)
//{
//    ComponentPool<PhysicsComponent, 1000>::Tick(gameClock);
//
//    //float stepInSeconds = time::FromTo<float>(gameClock.GetDeltaTime(), time::kMicrosecondUnit, time::kSecondUnit);
//    //mDynamicsWorld->stepSimulation(stepInSeconds);
//
//    //if (dev::CurrentConfiguration().mDebug.mFlags.Physics)
//    //{
//    //        //DBG_NoDebug = 0,
//    //        //DBG_DrawWireframe = 1,
//    //        //DBG_DrawAabb = 2,
//    //        //DBG_DrawFeaturesText = 4,
//    //        //DBG_DrawContactPoints = 8,
//    //        //DBG_NoDeactivation = 16,
//    //        //DBG_NoHelpText = 32,
//    //        //DBG_DrawText = 64,
//    //        //DBG_ProfileTimings = 128,
//    //        //DBG_EnableSatComparison = 256,
//    //        //DBG_DisableBulletLCP = 512,
//    //        //DBG_EnableCCD = 1024,
//    //        //DBG_DrawConstraints = (1 << 11),
//    //        //DBG_DrawConstraintLimits = (1 << 12),
//    //        //DBG_FastWireframe = (1 << 13),
//    //        //DBG_DrawNormals = (1 << 14),
//    //        //DBG_DrawFrames = (1 << 15),
//
//    //    //uint32_t flags = mBulletDebug.mWireFrame ? btIDebugDraw::DBG_DrawWireframe : 0;
//    //    //flags |= mBulletDebug.mAabb ? btIDebugDraw::DBG_DrawAabb : 0;
//    //    //mDebugDraw->setDebugMode(flags);
//    //    mDynamicsWorld->debugDrawWorld();
//    //}
//}

yaget::comp::PhysicsComponent::Params yaget::comp::physics::CreateInitState(int shape, const math3d::Vector3& /*halfExtents*/)
{
    comp::PhysicsComponent::Params params;

    switch (shape)
    {
    case BoxShape:

        //params.collisionShape = new btBoxShape(btVector3(halfExtents.x, halfExtents.y, halfExtents.z));// btScalar(8.0), btScalar(0.1), btScalar(8.0)));
        break;

    default:
        YAGET_ASSERT(false, "Shape '%d' for collision is not supported.", shape);
        break;
    }

    return params;
}
