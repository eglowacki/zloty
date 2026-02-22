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

#include "Render/Cache/CacheWatcher.h"

namespace yaget
{
    class DependencyGraph;
}


namespace yaget::render
{
    struct MaterialProperties
    {
        AssetCacheType mVertexShader = AssetCacheType::Empty;
        AssetCacheType mPixelShader = AssetCacheType::Empty;
        AssetCacheType mRasterizerState = AssetCacheType::Empty;
        AssetCacheType mBlendMode = AssetCacheType::Empty;
        AssetCacheType mDepthState = AssetCacheType::Empty;
        // this is calculated at run time after loading and reading above types
        AssetCacheType mSignature = AssetCacheType::Empty;
        AssetCacheType mPSO = AssetCacheType::Empty;

        bool operator == (MaterialProperties const&) const  = default;
    };

    class RenderMaterials : public CacheWatcher<MaterialProperties>
    {
    public:
        RenderMaterials(io::VirtualTransportSystem& vts);
        ~RenderMaterials();

        MaterialProperties GetMaterial(const io::Tag& tag);
        std::vector<MaterialProperties> GetMaterials(const io::Tags& tags);
    };
}
