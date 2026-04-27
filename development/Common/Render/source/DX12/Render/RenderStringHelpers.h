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

#include <nlohmann/json.hpp>
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
                result = "D3D_FEATURE_LEVEL_12_2";
                break;   
            case D3D_FEATURE_LEVEL_12_1:
                result = "D3D_FEATURE_LEVEL_12_1";
                break;
            case D3D_FEATURE_LEVEL_12_0:
                result = "D3D_FEATURE_LEVEL_12_0";
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


namespace DirectX
{
    //-------------------------------------------------------------------------------------------------
    //-------------------------------------------------------------------------------------------------
    inline void to_json(nlohmann::json& j, const XMFLOAT2& value)
    {
        j = yaget::conv::ToString(value);
    }


    //-------------------------------------------------------------------------------------------------
    inline void from_json(const nlohmann::json& j, XMFLOAT2& value)
    {
        std::string source;
        j.get_to(source);

        value = yaget::conv::FromString<XMFLOAT2>(source.c_str());
    }


    //-------------------------------------------------------------------------------------------------
    //-------------------------------------------------------------------------------------------------
    inline void to_json(nlohmann::json& j, const XMFLOAT3& value)
    {
        j = yaget::conv::ToString(value);
    }

    inline void from_json(const nlohmann::json& j, XMFLOAT3& value)
    {
        std::string source;
        j.get_to(source);

        value = yaget::conv::FromString<XMFLOAT3>(source.c_str());
    }


    //-------------------------------------------------------------------------------------------------
    //-------------------------------------------------------------------------------------------------
    inline void to_json(nlohmann::json& j, const XMFLOAT4& value)
    {
        j = yaget::conv::ToString(value);
    }


    //-------------------------------------------------------------------------------------------------
    inline void from_json(const nlohmann::json& j, XMFLOAT4& value)
    {
        std::string source;
        j.get_to(source);

        value = yaget::conv::FromString<XMFLOAT4>(source.c_str());
    }
    
}


namespace math3d
{
    //-------------------------------------------------------------------------------------------------
    //-------------------------------------------------------------------------------------------------
    inline void to_json(nlohmann::json& j, const Vector2& value)
    {
        to_json(j, static_cast<const DirectX::XMFLOAT2&>(value));
    }


    //-------------------------------------------------------------------------------------------------
    inline void from_json(const nlohmann::json& j, Vector2& value)
    {
        from_json(j, static_cast<DirectX::XMFLOAT2&>(value));
    }
    
    //-------------------------------------------------------------------------------------------------
    //-------------------------------------------------------------------------------------------------
    inline void to_json(nlohmann::json& j, const Vector3& value)
    {
        to_json(j, static_cast<const DirectX::XMFLOAT3&>(value));
    }


    //-------------------------------------------------------------------------------------------------
    inline void from_json(const nlohmann::json& j, Vector3& value)
    {
        from_json(j, static_cast<DirectX::XMFLOAT3&>(value));
    }


    //-------------------------------------------------------------------------------------------------
    //-------------------------------------------------------------------------------------------------
    inline void to_json(nlohmann::json& j, const Vector4& value)
    {
        to_json(j, static_cast<const DirectX::XMFLOAT4&>(value));
    }


    //-------------------------------------------------------------------------------------------------
    inline void from_json(const nlohmann::json& j, Vector4& value)
    {
        from_json(j, static_cast<DirectX::XMFLOAT4&>(value));
    }


    //-------------------------------------------------------------------------------------------------
    //-------------------------------------------------------------------------------------------------
    inline void to_json(nlohmann::json& j, const Color& value)
    {
        to_json(j, static_cast<const DirectX::XMFLOAT4&>(value));
    }


    //-------------------------------------------------------------------------------------------------
    inline void from_json(const nlohmann::json& j, Color& value)
    {
        from_json(j, static_cast<DirectX::XMFLOAT4&>(value));
    }


    //-------------------------------------------------------------------------------------------------
    //-------------------------------------------------------------------------------------------------
    inline void to_json(nlohmann::json& j, const Quaternion& value)
    {
        to_json(j, static_cast<const DirectX::XMFLOAT4&>(value));
    }


    //-------------------------------------------------------------------------------------------------
    inline void from_json(const nlohmann::json& j, Quaternion& value)
    {
        from_json(j, static_cast<DirectX::XMFLOAT4&>(value));
    }

}
