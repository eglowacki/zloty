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


namespace defensor::render
{
    using namespace yaget;

    //-------------------------------------------------------------------------------------------------
    class RenderMaterial
    {
    public:
        RenderMaterial(const io::Tag& assetTag);
        ~RenderMaterial();

        io::Tag mAssetTag;
    };

}
