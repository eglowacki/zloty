///////////////////////////////////////////////////////////////////////
// RenderMaterials.h
//
//  Copyright 02/14/2026 Edgar Glowacki.
//
//  Maintained by: Edgar
//
//  NOTES:
//
//  #include "Render/Pipeline/RenderMaterials.h"
//
//////////////////////////////////////////////////////////////////////
//! \file
#pragma once

//#include "Render/RenderCore.h"
//#include "Streams/Buffers.h"
#include "Render/Cache/CacheWatcher.h"

namespace yaget
{
    class DependencyGraph;
}

//struct ID3D12PipelineState;
//struct ID3D12RootSignature;
//struct ID3D12Device;


namespace yaget::render
{
    struct AssetTypes
    {
        AssetCacheType mVertexShader = AssetCacheType::Empty;
        AssetCacheType mPixelShader = AssetCacheType::Empty;
        AssetCacheType mRasterizerState = AssetCacheType::Empty;
        AssetCacheType mBlendMode = AssetCacheType::Empty;
        AssetCacheType mDepthState = AssetCacheType::Empty;
        // this is calculated at run time after loading and reading above types
        AssetCacheType mSignature = AssetCacheType::Empty;
        AssetCacheType mPSO = AssetCacheType::Empty;

        bool operator == (AssetTypes const&) const  = default;
    };

    class RenderMaterials : public CacheWatcher<AssetTypes>
    {
    public:
        RenderMaterials(io::VirtualTransportSystem& vts, DependencyGraph& dependencyGraph, io::Watcher& watcher);
        ~RenderMaterials();

        AssetTypes GetMaterial(const io::Tag& tag);
        std::vector<AssetTypes> GetMaterials(const io::Tags& tags);


        //ID3D12PipelineState* GetPipeline(const io::Tag& tag, ID3D12RootSignature* rootSignature, io::Buffer vertexShaderBuffer, io::Buffer pixelShaderBuffer);

    private:
    };
}
