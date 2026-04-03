#include "Render/Polygons/RenderShape.h"
#include "Render/Platform/D3D12MemAlloc.h"
#include <d3dx12.h>


namespace
{
    DXGI_FORMAT GetIndexFormat(size_t indexFormatSize)
    {
        switch (indexFormatSize)
        {
            case 2:
                return DXGI_FORMAT_R16_UINT;
            case 4:
                return DXGI_FORMAT_R32_UINT;
            default:
                return DXGI_FORMAT_UNKNOWN;
        }
    }
    
}
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

        if (mGeometryData.mHeader.mNumIndices)
        {
            auto indexFormat = GetIndexFormat(mGeometryData.mHeader.mIndexFormatSize);

            D3D12_INDEX_BUFFER_VIEW indexDataView{};
            indexDataView.BufferLocation = mGeometryData.mIndicesResource->GetGPUVirtualAddress();
            indexDataView.Format = indexFormat;
            indexDataView.SizeInBytes = mGeometryData.mHeader.IndexBufferSize();

             commandList->IASetIndexBuffer(&indexDataView);
             commandList->DrawIndexedInstanced(static_cast<uint32_t>(mGeometryData.mHeader.mNumIndices), 1, 0, 0, 0);
        }
        else
        {
            commandList->DrawInstanced(3 * numTriangles, 1, 0, 0);
        }
    }
}
