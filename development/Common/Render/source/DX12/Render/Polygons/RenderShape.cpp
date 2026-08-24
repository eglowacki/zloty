#include "Render/Polygons/RenderShape.h"
#include "Render/Platform/D3D12MemAlloc.h"


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
    if (geometryData != mGeometryData && geometryData.mHeader.IsValid())
    {
        *this = {};
        mGeometryData = std::move(geometryData);

        UpdateGeometryData();
    }
}


//-------------------------------------------------------------------------------------------------
void yaget::render::RenderShape::Render(ID3D12GraphicsCommandList* commandList, AssetCacheType psoType) const
{
    if (mGeometryData.mHeader.IsValid())
    {
        D3D_PRIMITIVE_TOPOLOGY topology = D3D_PRIMITIVE_TOPOLOGY_UNDEFINED;
        if (has(psoType, AssetCacheType::TopologyStateTriangle))
        {
            topology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
        }
        else if (has(psoType, AssetCacheType::TopologyStatePoint))
        {
            topology = D3D_PRIMITIVE_TOPOLOGY_POINTLIST;
        }
        else if (has(psoType, AssetCacheType::TopologyStateLine))
        {
            topology = D3D_PRIMITIVE_TOPOLOGY_LINELIST;
        }

        commandList->IASetPrimitiveTopology(topology);
        commandList->IASetVertexBuffers(0, 1, &mVertexBufferView);

        if (mHasIndices)
        {
             commandList->IASetIndexBuffer(&mIndexBufferView);
             commandList->DrawIndexedInstanced(static_cast<uint32_t>(mGeometryData.mHeader.mNumIndices), 1, 0, 0, 0);
        }
        else
        {
            commandList->DrawInstanced(3 * mNumTriangles, 1, 0, 0);
        }
    }
}


//-------------------------------------------------------------------------------------------------
void yaget::render::RenderShape::UpdateGeometryData()
{
    mVertexBufferView.BufferLocation = mGeometryData.mVerticesResource->GetGPUVirtualAddress();
    mVertexBufferView.SizeInBytes = mGeometryData.mHeader.VertexBufferSize();
    mVertexBufferView.StrideInBytes = mGeometryData.mHeader.VertexStride();

    if (mGeometryData.mHeader.mNumIndices)
    {
        mHasIndices = true;
        auto indexFormat = GetIndexFormat(mGeometryData.mHeader.mIndexFormatSize);

        mIndexBufferView.BufferLocation = mGeometryData.mIndicesResource->GetGPUVirtualAddress();
        mIndexBufferView.Format = indexFormat;
        mIndexBufferView.SizeInBytes = mGeometryData.mHeader.IndexBufferSize();
    }

    mNumTriangles = mGeometryData.mHeader.NumTriangles();
}
