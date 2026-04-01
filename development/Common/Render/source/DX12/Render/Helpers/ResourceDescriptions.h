///////////////////////////////////////////////////////////////////////
// ResourceDescriptions.h
//
//  Copyright 03/30/2026 Edgar Glowacki.
//
//  Maintained by: Edgar
//
//  NOTES:
//      
//
//  #include "Render/Helpers/ResourceDescriptions.h"
//
//////////////////////////////////////////////////////////////////////
//! \file
#pragma once

#include "Render/RenderCore.h"
#include "Streams/Buffers.h"

#include <d3dx12.h>
#include <dxgiformat.h>

struct ID3D12GraphicsCommandList;
struct ID3D12Resource;

namespace D3D12MA
{
    class Allocator;
    class Allocation;
}


namespace yaget::render::helpers
{
    D3D12MA::Allocation* CreateGpuHeap(const io::Tag& tag, int sizeX, int sizeY, D3D12_RESOURCE_DIMENSION dimension, D3D12_RESOURCE_STATES resourceState, D3D12_TEXTURE_LAYOUT layout, DXGI_FORMAT format, D3D12MA::Allocator* allocator);
    D3D12MA::Allocation* CreateUploadHeap(const io::Tag& tag, uint64_t bufferSize, D3D12MA::Allocator* allocator);

    void UploadData(ID3D12GraphicsCommandList* commandList, ID3D12Resource* destination, ID3D12Resource* intermediate, const uint8_t* data, int stride, size_t sliceSize);

    struct SourceGpuParameters
    {
        int mSizeX{};
        int mSizeY{};
        DXGI_FORMAT mFormat{};
        const uint8_t* mData{};
        int mStride{};
        size_t mSliceSize{};
        D3D12_RESOURCE_DIMENSION mDimension{};
        D3D12_TEXTURE_LAYOUT mLayout{};
        D3D12_RESOURCE_STATES mResourceState{};
        D3D12_RESOURCE_STATES mTransitionTo{};
    };


    struct GpuResourceResult
    {
        D3D12MA::Allocation* mGpuAllocation{};
        D3D12MA::Allocation* mUploadAllocation{};
    };

    GpuResourceResult CreateGpuResource(const io::Tag& tag, const SourceGpuParameters& parameters, ID3D12GraphicsCommandList* commandList, D3D12MA::Allocator* allocator);
    
}
