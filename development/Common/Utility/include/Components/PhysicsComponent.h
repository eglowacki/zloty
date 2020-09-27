/////////////////////////////////////////////////////////////////////////
// PhysicsComponent.h
//
//  Copyright 7/27/2016 Edgar Glowacki.
//
// NOTES:
//      Physiscs components, we are using Bullet under the hood
//
//
// #include "Components/PhysicsComponent.h"
//
/////////////////////////////////////////////////////////////////////////
//! \file
#pragma once

#include "MathFacade.h"
#include "Components/Component.h"
#include "Components/CollisionShape.h"
#include <atomic>

class btCollisionShape;
class btRigidBody;
class btDiscreteDynamicsWorld;
class btSequentialImpulseConstraintSolver;
class btBroadphaseInterface;
class btCollisionDispatcher;
class btDefaultCollisionConfiguration;
class btIDebugDraw;

namespace yaget
{
    namespace metrics { class Channel; }

    namespace comp
    {
        // forward declaration
        class LocationComponent;
        class PhysicsWorldComponent;
        class PhysicsComponent;

        namespace db
        {
            template <>
            struct ComponentProperties<comp::PhysicsWorldComponent>
            {
                using Row = std::tuple<>;
                using Types = std::tuple<>;
                static Types DefaultRow() { return Types{}; };
            };

            template <>
            struct ComponentProperties<comp::PhysicsComponent>
            {
                using Row = std::tuple<>;
                using Types = std::tuple<>;
                static Types DefaultRow() { return Types{}; };
            };

        }

        class PhysicsWorldComponent : public Component
        {
        public:
            static constexpr int Capacity = 4;

            PhysicsWorldComponent(Id_t id);

            void Update(const time::GameClock& gameClock, metrics::Channel& channel);

        private:
            size_t CalculateStateHash() const override { YAGET_ASSERT(false, "Implemented CalculateStateHash methos!!!"); return 0; }

            friend PhysicsComponent;

            //std::unique_ptr<btDefaultCollisionConfiguration> mCollisionConfiguration;
            //std::unique_ptr<btCollisionDispatcher> mDispatcher;
            //std::unique_ptr<btBroadphaseInterface> mOverlappingPairCache;
            //std::unique_ptr<btSequentialImpulseConstraintSolver> mSolver;
            //std::unique_ptr<btDiscreteDynamicsWorld> mDynamicsWorld;
            //std::unique_ptr<btIDebugDraw> mDebugDraw;
        };

        //! Any entity that needs to participate in physics
        class PhysicsComponent : public Component
        {
        public:
            using MetricPolicy = metrics::PerformancePolicy::Policy;
            static const uint32_t TransformChanged = "PhysicsComponent.TransformChanged"_crc32;

            struct Params
            {
                using WorldTransformCallback_t = std::function<void(const math3d::Vector3& position, const math3d::Quaternion& orientation)>;

                btCollisionShape* collisionShape = nullptr;
                const DirectX::SimpleMath::Matrix* matrix = nullptr;
                float mass = 0.0f;
                WorldTransformCallback_t worldTransformCallback;
            };

            PhysicsComponent(Id_t id, PhysicsWorldComponent* physicsWorld, const yaget::physics::CollisionShape& collisionSHape, comp::LocationComponent* locationComponent);
            PhysicsComponent(Id_t id, PhysicsWorldComponent* physicsWorld, const yaget::physics::CollisionShape& collisionSHape, const math3d::Vector3& position = math3d::Vector3(), const math3d::Quaternion& orientation = math3d::Quaternion());
            PhysicsComponent(Id_t id, PhysicsWorldComponent* physicsWorld, const Params& params);

            PhysicsComponent(Id_t id, btDiscreteDynamicsWorld* dynamicsWorld, const Params& params);
            virtual ~PhysicsComponent();
            virtual void Tick(const time::GameClock& gameClock) override;

            DirectX::SimpleMath::Matrix GetMatrix() const;
            void SetMatrix(const DirectX::SimpleMath::Matrix& matrix);

            // It may be perf gain to cache split matrix
            const math3d::Vector3& GetPosition() const { return mPosition; }
            const math3d::Quaternion& GetOrientation() const { return mOrientation; }

        private:
            size_t CalculateStateHash() const override { YAGET_ASSERT(false, "Implemented CalculateStateHash methos!!!"); return 0; }

            void onSigTransformChanged(const math3d::Vector3& position, const math3d::Quaternion& orientation);
            btDiscreteDynamicsWorld* mDynamicsWorld = nullptr;
            btCollisionShape* mColisionlShape = nullptr;
            btRigidBody* mRigidBody = nullptr;
            std::atomic_bool mUpdateDynamicsWorld{ false };

            // It may be perf gain to cache split matrix
            math3d::Vector3 mPosition;
            math3d::Quaternion mOrientation;
        };

        // DEPRECATED
        class PhysicsComponentPool : public ComponentPool<PhysicsComponent, 1000>
        {
        public:
            PhysicsComponentPool();
            virtual ~PhysicsComponentPool();

            virtual void Tick(const time::GameClock& gameClock) override;

            Ptr New(Id_t id, const PhysicsComponent::Params& params);

            void SetDebugDraw(btIDebugDraw* debugDraw);

        private:
            //std::unique_ptr<btDefaultCollisionConfiguration> mCollisionConfiguration;
            //std::unique_ptr<btCollisionDispatcher> mDispatcher;
            //std::unique_ptr<btBroadphaseInterface> mOverlappingPairCache;
            //std::unique_ptr<btSequentialImpulseConstraintSolver> mSolver;
            //std::unique_ptr<btDiscreteDynamicsWorld> mDynamicsWorld;
            //std::unique_ptr<btIDebugDraw> mDebugDraw;
        };

        namespace physics
        {
            constexpr int BoxShape = 1;

            PhysicsComponent::Params CreateInitState(int shape, const math3d::Vector3& halfExtents);

        } // namespace physics

    } // namespace comp

} // namespace yaget
