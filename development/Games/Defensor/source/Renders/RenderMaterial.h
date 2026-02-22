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
#include "Render/Pipeline/RenderMaterials.h"
#include "VTS/VirtualTransportSystem.h"


namespace defensor::render
{
    using namespace yaget;

    //-------------------------------------------------------------------------------------------------
    class RenderMaterial
    {
    public:
        using AssetCacheType = yaget::render::AssetCacheType;

        RenderMaterial(const io::Tag& assetTag, io::VirtualTransportSystem& vts);
        ~RenderMaterial();

        void ResolveAssetTag(const io::Tag& assetTag);

        io::Tag mAssetTag;
        io::VirtualTransportSystem& mVTS;

        using MaterialProperties = yaget::render::MaterialProperties;

        MaterialProperties mMaterialProperties;
    };

}
