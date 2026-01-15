#include "Renders/RenderComponent.h"
#include "Render/Platform/Adapter.h"
#include "Render/Polygons/Polygon.h"
#include <d3d12.h>


defensor::render::RenderComponent::RenderComponent(comp::Id_t id, const math3d::Matrix& matrix, const yaget::render::platform::Adapter& adapter)
    : BaseComponent(id)
    , mMatrix(matrix)
    , mPolygon(std::make_unique<yaget::render::Polygon>(adapter.GetDevice(), adapter.GetAllocator(), false /*useTwo*/))
{
}

defensor::render::RenderComponent::~RenderComponent()
{
}

void defensor::render::RenderComponent::Render(ID3D12GraphicsCommandList* commandList)
{
    mPolygon->Render(commandList, {});
}
