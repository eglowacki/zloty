///////////////////////////////////////////////////////////////////////
// RenderShape.h
//
//  Copyright 01/16/2026 Edgar Glowacki.
//
//  Maintained by: Edgar
//
//  NOTES:
//
//  #include "Render/Polygons/RenderShape.h"
//
//////////////////////////////////////////////////////////////////////
//! \file
#pragma once

#include "Render/Pipeline/RenderGeometries.h"
#include <d3dx12.h>

struct ID3D12GraphicsCommandList;
struct ID3D12Resource;

namespace D3D12MA
{
    class Allocator;
    class Allocation;
}


namespace yaget::render
{
    class RenderShape
    {
    public:
        RenderShape();
        ~RenderShape();

        void Bind(GeometriesResources::GeometryData geometryData);
        void Render(ID3D12GraphicsCommandList* commandList, AssetCacheType psoType) const;

    private:
        void UpdateGeometryData();

        GeometriesResources::GeometryData mGeometryData{};
        D3D12_VERTEX_BUFFER_VIEW mVertexBufferView{};
        D3D12_INDEX_BUFFER_VIEW mIndexBufferView{};
        uint32_t mNumTriangles{};
        bool mHasIndices = false;
    };

}
