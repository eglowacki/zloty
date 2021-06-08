#include "Components/LocationComponent.h"
#include "Debugging/Assert.h"
#include "Debugging/DevConfiguration.h"
#include "Logger/YLog.h"


//--------------------------------------------------------------------------------------------------
yaget::comp::LocationComponent::LocationComponent(Id_t id, const math3d::Vector3& position /*= math3d::Vector3()*/, const math3d::Quaternion& orientation /*= math3d::Quaternion()*/, const math3d::Vector3& scale /*= {1.0f, 1.0f, 1.0f }*/)
    : Component(id)
    , mPosition(position)
    , mOrientation(orientation)
    , mScale(scale)
{
    YAGET_ASSERT(dev::CurrentThreadIds().IsThreadLogic(), "Creation of component must be called from LOGIC thread.");
}

//--------------------------------------------------------------------------------------------------
yaget::comp::LocationComponent::~LocationComponent()
{
}

//--------------------------------------------------------------------------------------------------
size_t yaget::comp::LocationComponent::CalculateStateHash() const
{
    std::hash<LocationComponent> hash_fn;
    return hash_fn(*this);
}
