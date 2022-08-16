/////////////////////////////////////////////////////////////////////////
// RenderStringHelpers.h
//
//  Copyright 08/16/2022 Edgar Glowacki.
//
// NOTES:
//      Expose string conversion for various DX types
//
// #include "Render/RenderStringHelpers.h"
//
/////////////////////////////////////////////////////////////////////////
//! \file

#pragma once

#include "YagetCore.h"
#include "StringHelpers.h"
#include <d3dcommon.h>

namespace yaget::conv
{
    //-------------------------------------------------------------------------------------------------
    template <>
    struct Convertor<D3D_FEATURE_LEVEL>
    {
        static std::string ToString(D3D_FEATURE_LEVEL value)
        {
            std::string result;

            switch (value)
            {
            case D3D_FEATURE_LEVEL_12_2:
                result = "D3D Feature Level 12.2";
                break;
            case D3D_FEATURE_LEVEL_12_1:
                result = "D3D Feature Level 12.1";
                break;
            case D3D_FEATURE_LEVEL_12_0:
                result = "D3D Feature Level 12.0";
                break;
            default:
                result = fmt::format("Unknown Feature Level {:#x}", value);
                YLOG_ERROR("DEVI", "Invalid D3D_FEATURE_LEVEL value: '%s' conversion to string.", result.c_str());
            }

            return result;
        }
    };

}
