///////////////////////////////////////////////////////////////////////
// RenderSignatures.h
//
//  Copyright 01/16/2026 Edgar Glowacki.
//
//  Maintained by: Edgar
//
//  NOTES:
//
//  #include "Renders/RenderSignatures.h"
//
//////////////////////////////////////////////////////////////////////
//! \file
#pragma once

#include "Render/RenderCore.h"
#include "Render/Cache/CacheWatcher.h"
#include "Render/Pipeline/RenderShaders.h"


namespace yaget
{
    class DependencyGraph;
}

struct ID3D12RootSignature;
struct ID3D12Device;


namespace defensor::render
{
    using namespace yaget;

    class RenderSignatures : public yaget::render::CacheWatcher<yaget::render::ComPtr<ID3D12RootSignature>>
    {
    public:
        RenderSignatures(ID3D12Device* device, io::VirtualTransportSystem& vts);
        ~RenderSignatures();

        ID3D12RootSignature* GetSignature(const yaget::io::Tag& tag, const yaget::render::RenderShaders::RootDescResult& rootDescResult);
        ID3D12RootSignature* GetSignature(const yaget::io::Tag& tag);

    private:
        ID3D12Device* mDevice = {};
    };
}
