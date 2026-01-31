#include "Renders/RenderComponent.h"
#include "Render/Platform/Adapter.h"
#include "VTS/VirtualTransportSystem.h"
#include <d3d12.h>


//-------------------------------------------------------------------------------------------------
defensor::render::RenderComponent::RenderComponent(comp::Id_t id, const math3d::Matrix& matrix, const io::Tag& assetTag, io::VirtualTransportSystem& vts, const yaget::render::platform::Adapter& adapter)
    : BaseComponent(id)
    , mMatrix(matrix)
    , mRenderShape(adapter.GetAllocator(), assetTag, vts)
    , mRenderMaterial(assetTag, vts)
{
}


//-------------------------------------------------------------------------------------------------
defensor::render::RenderComponent::~RenderComponent() = default;


//-------------------------------------------------------------------------------------------------
void defensor::render::RenderComponent::Render(ID3D12GraphicsCommandList* commandList) const
{
    mRenderShape.Render(commandList);
}
