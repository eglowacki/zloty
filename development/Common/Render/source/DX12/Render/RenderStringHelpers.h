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
#include <VertexTypes.h>


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
                result = "Feature Level:         D3D_FEATURE_LEVEL_12_2";
                break;   
            case D3D_FEATURE_LEVEL_12_1:
                result = "Feature Level:         D3D_FEATURE_LEVEL_12_1";
                break;
            case D3D_FEATURE_LEVEL_12_0:
                result = "Feature Level:         D3D_FEATURE_LEVEL_12_0";
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
            return "Resource Binding:      " + FromEnumToString(value);
        }          
    };

    //-------------------------------------------------------------------------------------------------
    template <>
    struct Convertor<D3D12_RESOURCE_HEAP_TIER>
    {
        static std::string ToString(D3D12_RESOURCE_HEAP_TIER value)
        {
            return "Resource Heap:         " + FromEnumToString(value);
        }          
    };


    //-------------------------------------------------------------------------------------------------
    template <>
    struct Convertor<D3D_SHADER_MODEL>
    {
        static std::string ToString(D3D_SHADER_MODEL value)
        {
            return "Shader Model:          " + FromEnumToString(value);
        }                      
    };

    //-------------------------------------------------------------------------------------------------
    template <>
    struct Convertor<D3D_ROOT_SIGNATURE_VERSION>
    {
        static std::string ToString(D3D_ROOT_SIGNATURE_VERSION value)
        {
            return "Root Signature:        " + FromEnumToString(value);
        }
    };

    //-------------------------------------------------------------------------------------------------
    template <>
    struct Convertor<D3D12_RENDER_PASS_TIER>
    {
        static std::string ToString(D3D12_RENDER_PASS_TIER value)
        {
            return "Render Pass Tier:      " + FromEnumToString(value);
        }          
    };

    //-------------------------------------------------------------------------------------------------
    template <>
    struct Convertor<D3D12_RAYTRACING_TIER>
    {
        static std::string ToString(D3D12_RAYTRACING_TIER value)
        {
            return "Raytracing Tier:       " + FromEnumToString(value);
        }          
    };

    //-------------------------------------------------------------------------------------------------
    template <>
    struct Convertor<D3D12_MESH_SHADER_TIER>
    {
        static std::string ToString(D3D12_MESH_SHADER_TIER value)
        {
            return "Mesh Shader Tier:      " + FromEnumToString(value);
        }          
    };

    //-------------------------------------------------------------------------------------------------
    template <>
    struct Convertor<D3D12_COMMAND_LIST_TYPE>
    {
        static std::string ToString(D3D12_COMMAND_LIST_TYPE value)
        {
            return "Command List Type:     " + FromEnumToString(value);
        }
    };


    //-------------------------------------------------------------------------------------------------
    template <>
    struct Convertor<DirectX::XMFLOAT2>
    {
        // format: { 1.0f, 1.0f }
        static DirectX::XMFLOAT2 FromString(const char* value)
        {
            std::string strValue(value);
            std::erase(strValue, '{');
            std::erase(strValue, '}');

            DirectX::XMFLOAT2 vertex{}; 
            auto tokens = Split(strValue, ",", true);
            switch (tokens.size())
            {
                case 2:
                    vertex.y = conv::FromString<float>(tokens[1].c_str());
                    [[fallthrough]];
                case 1:
                    vertex.x = conv::FromString<float>(tokens[0].c_str());
                    [[fallthrough]];
            }

            return vertex;
        }
    };


    //-------------------------------------------------------------------------------------------------
    template <>
    struct Convertor<DirectX::XMFLOAT3>
    {
        // format: { 1.0f, 1.0f, 1.0f }
        static DirectX::XMFLOAT3 FromString(const char* value)
        {
            std::string strValue(value);
            std::erase(strValue, '{');
            std::erase(strValue, '}');

            DirectX::XMFLOAT3 vertex{}; 
            auto tokens = Split(strValue, ",", true);
            switch (tokens.size())
            {
                case 3:
                    vertex.z = conv::FromString<float>(tokens[2].c_str());
                    [[fallthrough]];
                case 2:
                    vertex.y = conv::FromString<float>(tokens[1].c_str());
                    [[fallthrough]];
                case 1:
                    vertex.x = conv::FromString<float>(tokens[0].c_str());
                    [[fallthrough]];
            }

            return vertex;
        }
    };


    //-------------------------------------------------------------------------------------------------
    template <>
    struct Convertor<DirectX::XMFLOAT4>
    {
        // format: { 1.0f, 1.0f, 1.0f, 1.0f }
        static DirectX::XMFLOAT4 FromString(const char* value)
        {
            std::string strValue(value);
            std::erase(strValue, '{');
            std::erase(strValue, '}');

            DirectX::XMFLOAT4 vertex{}; 
            auto tokens = Split(strValue, ",", true);
            switch (tokens.size())
            {
                case 4:
                    vertex.w = conv::FromString<float>(tokens[3].c_str());
                    [[fallthrough]];
                case 3:
                    vertex.z = conv::FromString<float>(tokens[2].c_str());
                    [[fallthrough]];
                case 2:
                    vertex.y = conv::FromString<float>(tokens[1].c_str());
                    [[fallthrough]];
                case 1:
                    vertex.x = conv::FromString<float>(tokens[0].c_str());
                    [[fallthrough]];
            }

            return vertex;
        }
    };


    //-------------------------------------------------------------------------------------------------
    template <>
    struct Convertor<DirectX::VertexPosition>
    {
        // format: { 1.0f, 1.0f, 1.0f }
        static DirectX::VertexPosition FromString(const char* value)
        {
            DirectX::VertexPosition vertex{}; 
            vertex.position = conv::FromString<DirectX::XMFLOAT3>(value);
            return vertex;
        }
    };


    //-------------------------------------------------------------------------------------------------
    template <>
    struct Convertor<DirectX::VertexPositionColor>
    {
        // format: { 1.0f, 1.0f, 1.0f }; { 1.0f, 0.0f, 0.0f, 1.0f }
        static DirectX::VertexPositionColor FromString(const char* value)
        {
            std::string strValue(value);

            DirectX::VertexPositionColor vertex{}; 
            auto tokens = Split(strValue, ";", true);
            switch (tokens.size())
            {
                case 2:
                    vertex.color = conv::FromString<DirectX::XMFLOAT4>(tokens[1].c_str());
                    [[fallthrough]];
                case 1:
                    vertex.position = conv::FromString<DirectX::XMFLOAT3>(tokens[0].c_str());
                    [[fallthrough]];
            }

            return vertex;
        }
    };


    //-------------------------------------------------------------------------------------------------
    template <>
    struct Convertor<DirectX::VertexPositionTexture>
    {
        // format: { 1.0f, 1.0f, 1.0f }; { 1.0f, 0.0f }
        static DirectX::VertexPositionTexture FromString(const char* value)
        {
            std::string strValue(value);

            DirectX::VertexPositionTexture vertex{}; 
            auto tokens = Split(strValue, ";", true);
            switch (tokens.size())
            {
                case 2:
                    vertex.textureCoordinate = conv::FromString<DirectX::XMFLOAT2>(tokens[1].c_str());
                    [[fallthrough]];
                case 1:
                    vertex.position = conv::FromString<DirectX::XMFLOAT3>(tokens[0].c_str());
                    [[fallthrough]];
            }

            return vertex;
        }
    };


    //-------------------------------------------------------------------------------------------------
    template <>
    struct Convertor<DirectX::VertexPositionColorTexture>
    {
        // format: { 1.0f, 1.0f, 1.0f }; { 1.0f, 0.0f, 0.0f, 1.0f }; { 1.0f, 0.0f }
        static DirectX::VertexPositionColorTexture FromString(const char* value)
        {
            std::string strValue(value);

            DirectX::VertexPositionColorTexture vertex{}; 
            auto tokens = Split(strValue, ";", true);
            switch (tokens.size())
            {
                case 3:
                    vertex.textureCoordinate = conv::FromString<DirectX::XMFLOAT2>(tokens[2].c_str());
                    [[fallthrough]];
                case 2:
                    vertex.color = conv::FromString<DirectX::XMFLOAT4>(tokens[1].c_str());
                    [[fallthrough]];
                case 1:
                    vertex.position = conv::FromString<DirectX::XMFLOAT3>(tokens[0].c_str());
                    [[fallthrough]];
            }

            return vertex;
        }
    };


} // namespace yaget::conv
