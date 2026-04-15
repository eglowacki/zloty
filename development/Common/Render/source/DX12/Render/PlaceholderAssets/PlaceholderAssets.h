///////////////////////////////////////////////////////////////////////
// PlaceholderAssets.h
//
//  Copyright 03/30/2026 Edgar Glowacki.
//
//  Maintained by: Edgar
//
//  NOTES:
//      
//
//  #include "Render/PlaceholderAssetsPlaceholderAssets.h"
//
//////////////////////////////////////////////////////////////////////
//! \file
#pragma once

#include "Render/RenderCore.h"
#include "ImageLoaders/ImageProcessor.h"

namespace yaget::render::placeholders
{
    //-------------------------------------------------------------------------------------------------
    struct Texture : public image::Header
    {
        io::BufferView mPixels;
    };

    //-------------------------------------------------------------------------------------------------
    Texture GetTextureData();

    //-------------------------------------------------------------------------------------------------
    io::BufferView GetShaderData();

    //-------------------------------------------------------------------------------------------------
    const Strings& GetGeometryData();

}
