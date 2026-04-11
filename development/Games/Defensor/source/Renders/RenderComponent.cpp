#include "Renders/RenderComponent.h"
#include "Render/Platform/Adapter.h"


//-------------------------------------------------------------------------------------------------
defensor::render::RenderComponent::RenderComponent(comp::Id_t id, const math3d::Matrix& matrix, const io::Tag& sceneItemTag)
    : BaseComponent(id)
    , mMatrix(matrix)
    , mSceneItemTag(sceneItemTag)
{
}


//-------------------------------------------------------------------------------------------------
defensor::render::RenderComponent::~RenderComponent() = default;
