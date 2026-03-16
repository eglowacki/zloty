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
        RenderMaterials(io::VirtualTransportSystem& vts, io::VirtualTransportSystem::Section fileName);
        ~RenderMaterials();

        MaterialProperties GetMaterial(const io::Tag& tag);
        std::vector<MaterialProperties> GetMaterials(const io::Tags& tags);

        static void PopulateMappings(io::VirtualTransportSystem::Section fileName, io::VirtualTransportSystem& vts);
        static void SaveMappings(io::VirtualTransportSystem::Section fileName, io::VirtualTransportSystem& vts);
    };
}



namespace yaget::conv
{
    template<>
    struct Convertor<yaget::render::MaterialProperties>
    {
        static std::string ToString(const yaget::render::MaterialProperties& value)
        {
            return std::format("Material Properties:\n\tvs:         '{}'\n\tps:         '{}'\n\tRasterizer: '{}'\n\tBlend:      '{}'\n\tDepth:      '{}'\n\tSignature   '{}'\n\tPipeline    '{}'.",
                render::internal::CacheTypeToString(value.mVertexShader),
                render::internal::CacheTypeToString(value.mPixelShader),
                render::internal::CacheTypeToString(value.mRasterizerState),
                render::internal::CacheTypeToString(value.mBlendMode),
                render::internal::CacheTypeToString(value.mDepthState),
                render::internal::CacheTypeToString(value.mSignature),
                render::internal::CacheTypeToString(value.mPSO));
        }
    };
}