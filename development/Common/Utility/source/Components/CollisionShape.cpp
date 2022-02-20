#include "Components/CollisionShape.h"

//YAGET_COMPILE_SUPPRESS_START(4127, "conditional expression is constant")
//YAGET_COMPILE_SUPPRESS_START(4099, "'': type name first seen using 'class' now seen using 'struct'")
//#include "btBulletDynamicsCommon.h" 
//YAGET_COMPILE_SUPPRESS_END
//YAGET_COMPILE_SUPPRESS_END;

yaget::physics::CollisionShape::CollisionShape(float mass)
    : mMass(mass)
{
}

float yaget::physics::CollisionShape::GetMass() const
{
    return mMass;
}

yaget::physics::BoxCollisionShape::BoxCollisionShape(const math3d::Vector3& /*halfExtends*/, float mass)
    : CollisionShape(mass)
    //, mCollisionShape(new btBoxShape(btVector3(halfExtends.x, halfExtends.y, halfExtends.z)))
{
}

btCollisionShape* yaget::physics::BoxCollisionShape::GetShape() const
{
    return mCollisionShape;
}
