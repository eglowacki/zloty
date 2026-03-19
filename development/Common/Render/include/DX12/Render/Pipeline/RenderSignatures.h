///////////////////////////////////////////////////////////////////////
// RenderSignatures.h
//
//  Copyright 01/16/2026 Edgar Glowacki.
//
//  Maintained by: Edgar
//
//  NOTES:
//
//  #include "Render/Pipeline/RenderSignatures.h"
//
//////////////////////////////////////////////////////////////////////
//! \file
#pragma once

#include "Render/RenderCore.h"
#include "Render/Cache/CacheWatcher.h"
#include "Render/Pipeline/RenderShaders.h"


struct ID3D12RootSignature;
struct ID3D12Device;


namespace yaget::render
{
    class RenderSignatures : public CacheWatcher<ComPtr<ID3D12RootSignature>>
    {
    public:
        RenderSignatures(ID3D12Device* device, io::VirtualTransportSystem& vts, io::VirtualTransportSystem::Section fileName);
        ~RenderSignatures();

        ID3D12RootSignature* GetSignature(const io::Tag& tag, const RenderShaders::RootDescResult& rootDescResult);
        ID3D12RootSignature* GetSignature(const io::Tag& tag);

        const RenderShaders::IndexMap& GetIndexMap(const io::Tag& tag);

        void ClearCache(const io::Tag& tag);

        static void PopulateMappings(io::VirtualTransportSystem::Section fileName, io::VirtualTransportSystem& vts);
        static void SaveMappings(io::VirtualTransportSystem::Section fileName, io::VirtualTransportSystem& vts);

    private:
        ID3D12Device* mDevice = {};

        using ShaderIndexMap = std::map<io::Tag, RenderShaders::IndexMap>;
        ShaderIndexMap mShaderIndexMap;
    };
}
