///////////////////////////////////////////////////////////////////////
// RenderPipeline.h
//
//  Copyright 01/16/2026 Edgar Glowacki.
//
//  Maintained by: Edgar
//
//  NOTES:
//
//  #include "Renders/RenderPipelines.h"
//
//////////////////////////////////////////////////////////////////////
//! \file
#pragma once

#include "Render/RenderCore.h"
#include "Streams/Buffers.h"
#include "Render/Cache/CacheWatcher.h"

namespace yaget
{
    class DependencyGraph;
}

struct ID3D12PipelineState;
struct ID3D12RootSignature;
struct ID3D12Device;


namespace defensor::render
{
    using namespace yaget;

    class RenderPipelines : public yaget::render::CacheWatcher<yaget::render::ComPtr<ID3D12PipelineState>>
    {
    public:
        RenderPipelines(ID3D12Device* device, io::VirtualTransportSystem& vts, DependencyGraph& dependencyGraph, io::Watcher& watcher);
        ~RenderPipelines();

        ID3D12PipelineState* GetPipeline(const io::Tag& tag, ID3D12RootSignature* rootSignature, io::Buffer vertexShaderBuffer, io::Buffer pixelShaderBuffer);

    private:
        ID3D12Device* mDevice = {};
    };
}
