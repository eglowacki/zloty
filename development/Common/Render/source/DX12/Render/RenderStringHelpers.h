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

#include <d3dx12.h>

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
                result = "Feature Level:         12.2";
                break;   
            case D3D_FEATURE_LEVEL_12_1:
                result = "Feature Level:         12.1";
                break;
            case D3D_FEATURE_LEVEL_12_0:
                result = "Feature Level:         12.0";
                break;
            default:
                auto vi = static_cast<int>(value);
                result = std::vformat("Unknown Feature Level {:#x}", std::make_format_args(vi));
                YLOG_ERROR("DEVI", "Invalid D3D_FEATURE_LEVEL value: '%s' conversion to string.", result.c_str());
            }

            return result;
        }
    };

    //-------------------------------------------------------------------------------------------------
    template <>
    struct Convertor<D3D12_RESOURCE_BINDING_TIER>
    {
        static std::string ToString(D3D12_RESOURCE_BINDING_TIER value)
        {
            return "Resource Binding:Tier: " + std::to_string(static_cast<int>(value));
        }          
    };

    //-------------------------------------------------------------------------------------------------
    template <>
    struct Convertor<D3D12_RESOURCE_HEAP_TIER>
    {
        static std::string ToString(D3D12_RESOURCE_HEAP_TIER value)
        {
            return "Resource Heap:Tier:    " + std::to_string(static_cast<int>(value));
        }          
    };


    //-------------------------------------------------------------------------------------------------
    template <>
    struct Convertor<D3D_SHADER_MODEL>
    {
        static std::string ToString(D3D_SHADER_MODEL value)
        {
            return std::string("Shader Model:          ") + std::format("{:x}", static_cast<int>(value));
        }                      
    };

    //-------------------------------------------------------------------------------------------------
    template <>
    struct Convertor<D3D_ROOT_SIGNATURE_VERSION>
    {
        static std::string ToString(D3D_ROOT_SIGNATURE_VERSION value)
        {
            std::string result;

            switch (value)
            {
            case D3D_ROOT_SIGNATURE_VERSION_1_0:
                result = "Root Signature ven:    1.0";
                break;
            case D3D_ROOT_SIGNATURE_VERSION_1_1:
                result = "Root Signature ver:    1.1";
                break;
            case D3D_ROOT_SIGNATURE_VERSION_1_2:
                result = "Root Signature ver:    1.2";
                break;   
            default:
                auto vi = static_cast<int>(value);
                result = std::vformat("Unknown Root Signature Version {:#x}", std::make_format_args(vi));
                YLOG_ERROR("DEVI", "Invalid D3D_ROOT_SIGNATURE_VERSION value: '%s' conversion to string.", result.c_str());
            }

            return result;
        }
    };

    //-------------------------------------------------------------------------------------------------
    template <>
    struct Convertor<D3D12_RENDER_PASS_TIER>
    {
        static std::string ToString(D3D12_RENDER_PASS_TIER value)
        {
            return "Render Pass Tier:      " + std::to_string(static_cast<int>(value));
        }          
    };

    //-------------------------------------------------------------------------------------------------
    template <>
    struct Convertor<D3D12_RAYTRACING_TIER>
    {
        static std::string ToString(D3D12_RAYTRACING_TIER value)
        {
            return "Raytracing Tier:       " + std::to_string(static_cast<int>(value));
        }          
    };

    //-------------------------------------------------------------------------------------------------
    template <>
    struct Convertor<D3D12_MESH_SHADER_TIER>
    {
        static std::string ToString(D3D12_MESH_SHADER_TIER value)
        {
            return "Mesh Shader Tier:      " + std::to_string(static_cast<int>(value));
        }          
    };

    //-------------------------------------------------------------------------------------------------
    template <>
    struct Convertor<D3D12_COMMAND_LIST_TYPE>
    {
        static std::string ToString(D3D12_COMMAND_LIST_TYPE value)
        {
            std::string result;

            switch (value)
            {
            case D3D12_COMMAND_LIST_TYPE_DIRECT:
                result = "Command List Type:     Direct";
                break;   
            case D3D12_COMMAND_LIST_TYPE_BUNDLE:
                result = "Command List Type:     Bundle";
                break;
            case D3D12_COMMAND_LIST_TYPE_COMPUTE:
                result = "Command List Type:     Compute";
                break;
            case D3D12_COMMAND_LIST_TYPE_COPY:
                result = "Command List Type:     Copy";
                break;
            case D3D12_COMMAND_LIST_TYPE_VIDEO_DECODE:
                result = "Command List Type:     Decode";
                break;
            case D3D12_COMMAND_LIST_TYPE_VIDEO_PROCESS:
                result = "Command List Type:     Process";
                break;
            case D3D12_COMMAND_LIST_TYPE_VIDEO_ENCODE:
                result = "Command List Type:     Encode";
                break;
            case D3D12_COMMAND_LIST_TYPE_NONE:
                result = "Command List Type:     None";
                break;
            default:
                auto vi = static_cast<int>(value);
                result = std::vformat("Unknown Mesh Shader Tier {:#x}", std::make_format_args(vi));
                YLOG_ERROR("DEVI", "Invalid D3D12_COMMAND_LIST_TYPE_DIRECT value: '%s' conversion to string.", result.c_str());
            }

            return result;
        }
    };

} // namespace yaget::conv
