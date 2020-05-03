#include "Components/LocationComponent.h"
#include "Debugging/Assert.h"
#include "Debugging/DevConfiguration.h"
#include "Logger/YLog.h"


//--------------------------------------------------------------------------------------------------
yaget::comp::LocationComponent::LocationComponent(Id_t id, const math3d::Vector3& position /*= math3d::Vector3()*/, const math3d::Quaternion& orientation /*= math3d::Quaternion()*/, const math3d::Vector3& scale /*= math3d::Vector3::One*/)
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

//--------------------------------------------------------------------------------------------------
typename yaget::comp::LocationComponentPool::Ptr yaget::comp::LocationComponentPool::New(yaget::comp::Id_t id)
{
    Ptr c = NewComponent(id);
    return c;
}

//--------------------------------------------------------------------------------------------------
size_t yaget::comp::LocationComponentPool::TokenizeState(char* buffer, size_t size, size_t stride, std::function<bool(yaget::comp::Id_t id)> isValidCB)
{
    if (!buffer)
    {
        size_t totalComponenents = 0;
        ForEach([&totalComponenents, isValidCB](LocationComponent* component)
        {
            if (isValidCB(component->Id()))
            {
                totalComponenents++;
            }
            return true;
        });

        return totalComponenents;
    }

    size_t currentIndex = 0;
    ForEach([buffer, size, stride, &currentIndex, isValidCB](LocationComponent* component)
    {
        if (isValidCB(component->Id()))
        {
            TokenState* tokenState = reinterpret_cast<TokenState*>(&buffer[currentIndex * stride]);
            currentIndex++;
            tokenState->mId = component->Id();
            tokenState->mPosition = component->GetPosition();
            tokenState->mOrientation = component->GetOrientation();

            if (currentIndex == size)
            {
                return false;
            }
        }

        return true;
    });

    return NumComponents();
}

