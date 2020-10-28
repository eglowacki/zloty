#include "RenderComponent.h"

ttt::RenderComponent::RenderComponent(yaget::comp::Id_t id, yaget::io::Tags tags)
    : yaget::comp::BaseComponent<yaget::comp::DefaultPoolSize>(id)
    , mTags(std::move(tags))
{
}
