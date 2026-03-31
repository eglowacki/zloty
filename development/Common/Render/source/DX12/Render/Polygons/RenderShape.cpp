#include "Render/Polygons/RenderShape.h"
#include "Core/ErrorHandlers.h"
#include "Render/Platform/DeviceDebugger.h"
#include "Render/Platform/D3D12MemAlloc.h"
#include "Streams/Buffers.h"
#include "MathFacade.h"
#include <d3dx12.h>
#include <VertexTypes.h>


//-------------------------------------------------------------------------------------------------
yaget::render::RenderShape::RenderShape() = default;


//-------------------------------------------------------------------------------------------------
yaget::render::RenderShape::~RenderShape() = default;


//-------------------------------------------------------------------------------------------------
void yaget::render::RenderShape::Bind(GeometriesResources::GeometryData geometryData)
{
    mGeometryData = std::move(geometryData);
}


//-------------------------------------------------------------------------------------------------
void yaget::render::RenderShape::Render(ID3D12GraphicsCommandList* commandList) const
{
    if (mGeometryData.mHeader.IsValid())
    {
        D3D12_VERTEX_BUFFER_VIEW vertexDataView{};
        vertexDataView.BufferLocation = mGeometryData.mVerticesResource->GetGPUVirtualAddress();
        vertexDataView.SizeInBytes = mGeometryData.mHeader.VertexBufferSize();
        vertexDataView.StrideInBytes = mGeometryData.mHeader.VertexStride();

        auto numTriangles = mGeometryData.mHeader.NumTriangles();

        commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        commandList->IASetVertexBuffers(0, 1, &vertexDataView);
        commandList->DrawInstanced(3 * numTriangles, 1, 0, 0);
    }
}
