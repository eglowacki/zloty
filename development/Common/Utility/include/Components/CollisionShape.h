//////////////////////////////////////////////////////////////////////
// CollisionShape.h
//
//  Copyright 6/22/2019 Edgar Glowacki
//
//  Maintained by: Edgar
//
//  NOTES:
//      Provides sdk independent collision shape
//
//
//  #include "Components/CollisionShape.h"
//
//////////////////////////////////////////////////////////////////////
//! \file
#pragma once

#include "YagetCore.h"
#include "MathFacade.h"

class btCollisionShape;

namespace yaget
{
    namespace physics
    {
        class CollisionShape
        {
        public:
            virtual ~CollisionShape() {}
            virtual btCollisionShape* GetShape() const = 0;

            float GetMass() const;

        protected:
            CollisionShape(float mass);

        private:
            float mMass = 0.0f;
        };

        // This will not cleanup pointer to mCollisionShape
        // It is user responsibility to take ownership of mCollisionShape
        // pointer and manage it's lifetime
        class BoxCollisionShape : public CollisionShape
        {
        public:
            BoxCollisionShape(const math3d::Vector3& halfExtends, float mass);
            virtual ~BoxCollisionShape() {}

            btCollisionShape* GetShape() const override;

        private:
            btCollisionShape* mCollisionShape = nullptr;
        };

    } // namespace physics
} // namespace yaget
