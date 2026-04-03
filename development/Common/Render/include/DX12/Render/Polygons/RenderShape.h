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

//#include <d3d12.h>

#include "Render/RenderCore.h"
#include "Render/Pipeline/RenderGeometries.h"
#include "Streams/Buffers.h"

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
        void Render(ID3D12GraphicsCommandList* commandList) const;

    private:
        GeometriesResources::GeometryData mGeometryData{};
    };

}
