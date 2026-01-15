#include "Renders/SceneComponent.h"
#include <ranges>


defensor::render::SceneComponent::SceneComponent(comp::Id_t id)
    : BaseComponent(id)
{
}

const defensor::render::EntityState* defensor::render::SceneComponent::FindState(comp::Id_t id) const
{
    const auto result = std::ranges::find_if(mEntities, [id](const auto& element)
    {
        return element.mId == id;
    });

    return result != mEntities.end() ? &(*result) : nullptr;
}


yaget::comp::ItemIds defensor::render::SceneComponent::GetIds() const
{
    comp::ItemIds ids = 
    mEntities | 
    std::views::transform([](const auto& e)
    {
        return e.mId;
    }) | 
    std::ranges::to<std::set>();

    return ids;
}
