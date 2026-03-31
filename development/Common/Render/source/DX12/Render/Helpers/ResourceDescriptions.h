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
    D3D12MA::Allocation* CreateCopyHeapTexture(const io::Tag& tag, int sizeX, int sizeY, DXGI_FORMAT format, D3D12MA::Allocator* allocator);
    D3D12MA::Allocation* CreateUploadHeapTexture(const io::Tag& tag, uint64_t bufferSize, D3D12MA::Allocator* allocator);

    void UploadData(ID3D12GraphicsCommandList* commandList, ID3D12Resource* destination, ID3D12Resource* intermediate, const uint8_t* data, int stride, size_t sliceSize);

    //D3D12MA::Allocation* CreateTextureResource(const io::Tag& tag, int sizeX, int sizeY, DXGI_FORMAT format,         ID3D12GraphicsCommandList* commandList);

    
}
