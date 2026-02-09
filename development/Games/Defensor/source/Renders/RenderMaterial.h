///////////////////////////////////////////////////////////////////////
// RenderMaterial.h
//
//  Copyright 01/30/2026 Edgar Glowacki.
//
//  Maintained by: Edgar
//
//  NOTES:
//
//  #include "Renders/RenderMaterial.h"
//
//////////////////////////////////////////////////////////////////////
//! \file
#pragma once

#include "Render/Cache/AssetCache.h"
#include "VTS/VirtualTransportSystem.h"


namespace defensor::render
{
    using namespace yaget;

    //-------------------------------------------------------------------------------------------------
    class RenderMaterial
    {
    public:
        RenderMaterial(const io::Tag& assetTag, io::VirtualTransportSystem& vts);
        ~RenderMaterial();

        void ResolveAssetTag(const io::Tag& assetTag);

        io::Tag mAssetTag;
        io::VirtualTransportSystem& mVTS;

        yaget::render::AssetCacheType mVertexShader = yaget::render::AssetCacheType::Empty;
        yaget::render::AssetCacheType mPixelShader = yaget::render::AssetCacheType::Empty;
        yaget::render::AssetCacheType mRasterizerState = yaget::render::AssetCacheType::Empty;
        yaget::render::AssetCacheType mBlendMode = yaget::render::AssetCacheType::Empty;
        yaget::render::AssetCacheType mDepthState = yaget::render::AssetCacheType::Empty;
    };

}
