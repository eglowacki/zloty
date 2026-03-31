#include "Renders/RenderComponent.h"
#include "Render/Platform/Adapter.h"


//-------------------------------------------------------------------------------------------------
defensor::render::RenderComponent::RenderComponent(comp::Id_t id, const math3d::Matrix& matrix, const io::Tag& geometryTag, const io::Tag& materialTag, const io::Tags& textureTags)
    : BaseComponent(id)
    , mMatrix(matrix)
    , mRenderShape()
    , mRenderMaterial(materialTag)
    , mGeometryTag(geometryTag)
    , mTextureTags(textureTags)
{
}


//-------------------------------------------------------------------------------------------------
defensor::render::RenderComponent::~RenderComponent() = default;


//-------------------------------------------------------------------------------------------------
void defensor::render::RenderComponent::Bind(yaget::render::GeometriesResources::GeometryData geometryData)
{
    mRenderShape.Bind(geometryData);
}


//-------------------------------------------------------------------------------------------------
void defensor::render::RenderComponent::Render(ID3D12GraphicsCommandList* commandList) const
{
    mRenderShape.Render(commandList);
}
