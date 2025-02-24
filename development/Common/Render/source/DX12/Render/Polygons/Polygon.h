/////////////////////////////////////////////////////////////////////////
// Polygon.h
//
//  Copyright 08/15/2022 Edgar Glowacki.
//
// NOTES:
//      First attempt at creating a renderable object in DX12, like a triangle or a model composed of triangles
//
// #include "Render/Polygons/Polygon.h"
//
/////////////////////////////////////////////////////////////////////////
//! \file

#pragma once

#include "Render/RenderCore.h"

#include <functional>

struct ID3D12CommandAllocator;
struct ID3D12Device;
struct ID3D12GraphicsCommandList;
struct ID3D12PipelineState;
struct ID3D12RootSignature;

namespace D3D12MA
{
    class Allocation;
    class Allocator;
}

namespace yaget::render
{
    class Polygon
    {
    public:
        Polygon(ID3D12Device* device, D3D12MA::Allocator* allocator, bool useTwo);
        ~Polygon();

        ID3D12GraphicsCommandList* Render(ID3D12GraphicsCommandList* commandList, std::function<void(ID3D12GraphicsCommandList* commandList)> setup);

    private:
        ComPtr<ID3D12RootSignature> mRootSignature;
        ComPtr<ID3D12PipelineState> mPipelineState;
        unique_obj<D3D12MA::Allocation> mAllocation;
    };
}
