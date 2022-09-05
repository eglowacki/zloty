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
#include "Render/Platform/CommandQueue.h"

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
                result = "Feature Level: 12.2";
                break;
            case D3D_FEATURE_LEVEL_12_1:
                result = "Feature Level: 12.1";
                break;
            case D3D_FEATURE_LEVEL_12_0:
                result = "Feature Level: 12.0";
                break;
            default:
                result = fmt::format("Unknown Feature Level {:#x}", value);
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
            std::string result;

            switch (value)
            {
            case D3D12_RESOURCE_BINDING_TIER_1:
                result = "Resource Binding: Tier 1";
                break;
            case D3D12_RESOURCE_BINDING_TIER_2:
                result = "Resource Binding: Tier 2";
                break;
            case D3D12_RESOURCE_BINDING_TIER_3:
                result = "Resource Binding: Tier 3";
                break;
            default:
                result = fmt::format("Unknown Resource Binding Level {:#x}", value);
                YLOG_ERROR("DEVI", "Invalid D3D12_RESOURCE_BINDING_TIER value: '%s' conversion to string.", result.c_str());
            }

            return result;
        }
    };

    //-------------------------------------------------------------------------------------------------
    template <>
    struct Convertor<D3D_SHADER_MODEL>
    {
        static std::string ToString(D3D_SHADER_MODEL value)
        {
            std::string result;

            switch (value)
            {
            case D3D_SHADER_MODEL_5_1:
                result = "Shader Model: 5.1";
                break;
            case D3D_SHADER_MODEL_6_0:
                result = "Shader Model: 6.0";
                break;
            case D3D_SHADER_MODEL_6_1:
                result = "Shader Model: 6.1";
                break;
            case D3D_SHADER_MODEL_6_2:
                result = "Shader Model: 6.2";
                break;
            case D3D_SHADER_MODEL_6_3:
                result = "Shader Model: 6.3";
                break;
            case D3D_SHADER_MODEL_6_4:
                result = "Shader Model: 6.4";
                break;
            case D3D_SHADER_MODEL_6_5:
                result = "Shader Model: 6.5";
                break;
            case D3D_SHADER_MODEL_6_6:
                result = "Shader Model: 6.6";
                break;
            case D3D_SHADER_MODEL_6_7:
                result = "Shader Model: 6.7";
                break;
            case D3D_SHADER_MODEL_6_8:
                result = "Shader Model: 6.8";
                break;
            default:
                result = fmt::format("Unknown Shader Model {:#x}", value);
                YLOG_ERROR("DEVI", "Invalid D3D_SHADER_MODEL value: '%s' conversion to string.", result.c_str());
            }

            return result;
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
            case D3D_ROOT_SIGNATURE_VERSION_1:
                result = "Root Signature Version: 1.0";
                break;
            case D3D_ROOT_SIGNATURE_VERSION_1_1:
                result = "Root Signature Version: 1.1";
                break;
            default:
                result = fmt::format("Unknown Root Signature Version {:#x}", value);
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
            std::string result;

            switch (value)
            {
            case D3D12_RENDER_PASS_TIER_0:
                result = "Render Pass: Tier 0";
                break;
            case D3D12_RENDER_PASS_TIER_1:
                result = "Render Pass: Tier 1";
                break;
            case D3D12_RENDER_PASS_TIER_2:
                result = "Render Pass: Tier 2";
                break;
            default:
                result = fmt::format("Unknown Render Pass Tier {:#x}", value);
                YLOG_ERROR("DEVI", "Invalid D3D12_RENDER_PASS_TIER value: '%s' conversion to string.", result.c_str());
            }

            return result;
        }
    };

    //-------------------------------------------------------------------------------------------------
    template <>
    struct Convertor<D3D12_RAYTRACING_TIER>
    {
        static std::string ToString(D3D12_RAYTRACING_TIER value)
        {
            std::string result;

            switch (value)
            {
            case D3D12_RAYTRACING_TIER_NOT_SUPPORTED:
                result = "Raytracing Not Supported";
                break;
            case D3D12_RAYTRACING_TIER_1_0:
                result = "Raytracing Tier Supported 1.0";
                break;
            case D3D12_RAYTRACING_TIER_1_1:
                result = "Raytracing Tier Supported 1.1";
                break;
            default:
                result = fmt::format("Unknown Render Pass Tier {:#x}", value);
                YLOG_ERROR("DEVI", "Invalid D3D12_RAYTRACING_TIER value: '%s' conversion to string.", result.c_str());
            }

            return result;
        }
    };

    //-------------------------------------------------------------------------------------------------
    template <>
    struct Convertor<D3D12_MESH_SHADER_TIER>
    {
        static std::string ToString(D3D12_MESH_SHADER_TIER value)
        {
            std::string result;

            switch (value)
            {
            case D3D12_MESH_SHADER_TIER_NOT_SUPPORTED:
                result = "Mesh Shader Not Supported";
                break;
            case D3D12_MESH_SHADER_TIER_1:
                result = "Mesh Shader: Tier 1";
                break;
            default:
                result = fmt::format("Unknown Mesh Shader Tier {:#x}", value);
                YLOG_ERROR("DEVI", "Invalid D3D12_MESH_SHADER_TIER value: '%s' conversion to string.", result.c_str());
            }

            return result;
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
                result = "Command List Type: Direct";
                break;
            case D3D12_COMMAND_LIST_TYPE_BUNDLE:
                result = "Command List Type: Bundle";
                break;
            case D3D12_COMMAND_LIST_TYPE_COMPUTE:
                result = "Command List Type: Compute";
                break;
            case D3D12_COMMAND_LIST_TYPE_COPY:
                result = "Command List Type: Copy";
                break;
            case D3D12_COMMAND_LIST_TYPE_VIDEO_DECODE:
                result = "Command List Type: Decode";
                break;
            case D3D12_COMMAND_LIST_TYPE_VIDEO_PROCESS:
                result = "Command List Type: Process";
                break;
            case D3D12_COMMAND_LIST_TYPE_VIDEO_ENCODE:
                result = "Command List Type: Encode";
                break;
            case D3D12_COMMAND_LIST_TYPE_NONE:
                result = "Command List Type: None";
                break;
            default:
                result = fmt::format("Unknown Mesh Shader Tier {:#x}", value);
                YLOG_ERROR("DEVI", "Invalid D3D12_COMMAND_LIST_TYPE_DIRECT value: '%s' conversion to string.", result.c_str());
            }

            return result;
        }
    };

    //-------------------------------------------------------------------------------------------------
    template <>
    struct Convertor<yaget::render::platform::CommandQueue::Type>
    {
        static std::string ToString(yaget::render::platform::CommandQueue::Type value)
        {
            std::string result;

            switch (value)
            {
            case yaget::render::platform::CommandQueue::Type::Direct:
                result = "Command Direct";
                break;
            case yaget::render::platform::CommandQueue::Type::Compute:
                result = "Command Compute";
                break;
            case yaget::render::platform::CommandQueue::Type::Copy:
                result = "Command Copy";
                break;
            default:
                result = fmt::format("Unknown CommandQueue type {:#x}", value);
                YLOG_ERROR("DEVI", "Invalid CommandQueue::Type value: '%s' conversion to string.", result.c_str());
            }

            return result;
        }
    };

} // namespace yaget::conv
