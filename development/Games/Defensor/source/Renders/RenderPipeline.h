///////////////////////////////////////////////////////////////////////
// RenderPipeline.h
//
//  Copyright 01/16/2026 Edgar Glowacki.
//
//  Maintained by: Edgar
//
//  NOTES:
//
//  #include "Renders/RenderPipeline.h"
//
//////////////////////////////////////////////////////////////////////
//! \file
#pragma once

#include "Render/RenderCore.h"

struct ID3D12PipelineState;
struct ID3D12RootSignature;
struct ID3D12Device;


namespace defensor::render
{
    class RenderPipeline
    {
    public:
        RenderPipeline(ID3D12Device* device);
        ~RenderPipeline();

        ID3D12PipelineState* GetPipeline(uint64_t pipeType, ID3D12RootSignature* rootSignature);

    private:
        ID3D12Device* mDevice = {};
        std::map<uint64_t, yaget::render::ComPtr<ID3D12PipelineState>> mPipelines;
    };
}
