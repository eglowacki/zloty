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

#include <d3d12.h>

#include "Render/RenderCore.h"
#include "Streams/Buffers.h"

namespace yaget::io
{
    class VirtualTransportSystem;
}

namespace D3D12MA
{
    class Allocator;
    class Allocation;
}
struct ID3D12GraphicsCommandList;


namespace yaget::render
{
    class RenderShape
    {
    public:
        RenderShape(D3D12MA::Allocator* allocator, const io::Tag& assetTag, io::VirtualTransportSystem& vts);
        ~RenderShape();

        void UploadData(const io::Buffer& dataBlock);
        void Render(ID3D12GraphicsCommandList* commandList) const;

    private:
        unique_obj<D3D12MA::Allocation> mAllocation;
        uint32_t mVertexBufferSize; // in bytes
    };

}
