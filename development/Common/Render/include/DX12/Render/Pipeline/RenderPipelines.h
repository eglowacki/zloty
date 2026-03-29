///////////////////////////////////////////////////////////////////////
// RenderPipeline.h
//
//  Copyright 01/16/2026 Edgar Glowacki.
//
//  Maintained by: Edgar
//
//  NOTES:
//
//  #include "Render/Pipeline/RenderPipelines.h"
//
//////////////////////////////////////////////////////////////////////
//! \file
#pragma once


#include "Render/RenderCore.h"
#include "Streams/Buffers.h"
#include "Render/Cache/CacheWatcher.h"
#include "RenderShaders.h"

namespace yaget
{
    class DependencyGraph;
}

struct ID3D12PipelineState;
struct ID3D12RootSignature;
struct ID3D12Device;


namespace yaget::render
{
    class RenderPipelines : public CacheWatcher<ComPtr<ID3D12PipelineState>>
    {
    public:
        RenderPipelines(ID3D12Device* device, io::VirtualTransportSystem& vts, io::VirtualTransportSystem::Section fileName);
        ~RenderPipelines();

        ID3D12PipelineState* GetPipeline(const io::Tag& tag, ID3D12RootSignature* rootSignature,
                                         io::Buffer vertexShaderBuffer, const RenderShaders::ShaderInputOutputPins& vertexPins,
                                         io::Buffer pixelShaderBuffer, const RenderShaders::ShaderInputOutputPins& pixelPins);
        ID3D12PipelineState* GetPipeline(const io::Tag& tag);

        static void PopulateMappings(io::VirtualTransportSystem::Section fileName, io::VirtualTransportSystem& vts);
        static void SaveMappings(io::VirtualTransportSystem::Section fileName, io::VirtualTransportSystem& vts);

    private:
        ID3D12Device* mDevice = {};
    };
}
