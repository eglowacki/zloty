#include "Render/Polygons/RenderShape.h"
#include "Core/ErrorHandlers.h"
#include "Render/Platform/DeviceDebugger.h"
#include "Render/Platform/D3D12MemAlloc.h"
#include "Streams/Buffers.h"
#include "MathFacade.h"
#include <d3dx12.h>
#include <VertexTypes.h>


//-------------------------------------------------------------------------------------------------
yaget::render::RenderShape::RenderShape()
{
}


//-------------------------------------------------------------------------------------------------
yaget::render::RenderShape::~RenderShape() = default;


//-------------------------------------------------------------------------------------------------
void yaget::render::RenderShape::Bind(ID3D12Resource* resource)
{
    mResource = resource;
    if (mResource)
    {
        mVertexBufferSize = static_cast<uint32_t>(mResource->GetDesc().Width);
    }
    else
    {
        mVertexBufferSize = 0;
    }
}


//-------------------------------------------------------------------------------------------------
void yaget::render::RenderShape::Render(ID3D12GraphicsCommandList* commandList) const
{
    //auto resource = mAllocation->GetResource();

    if (mResource)
    {
        auto vertexFormatSize = 36; // sizeof(DirectX::VertexPositionColorTexture)
        auto numTriangles = 1; //mVertexBufferSize / (3 * vertexFormatSize);
        D3D12_VERTEX_BUFFER_VIEW vertexDataView;
        vertexDataView.BufferLocation = mResource->GetGPUVirtualAddress();
        vertexDataView.SizeInBytes = mVertexBufferSize;
        vertexDataView.StrideInBytes = vertexFormatSize;

        commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        commandList->IASetVertexBuffers(0, 1, &vertexDataView);
        commandList->DrawInstanced(3 * numTriangles, 1, 0, 0);
    }
}
