//////////////////////////////////////////////////////////////////////
// RenderConfigAssets.h
//
//  Copyright 9/7/2019 Edgar Glowacki
//
//  Maintained by: Edgar
//
//  NOTES:
//      Provides render/device configuration files
//
//
//  #include "VTS/RenderConfigAssets.h"
//
//////////////////////////////////////////////////////////////////////
//! \file
#pragma once

#include "YagetCore.h"
#include "VTS/RenderResolvedAssets.h"


namespace yaget
{
    namespace io::render
    {
        //-------------------------------------------------------------------------------------------------------------------------------
        // D3D_DRIVER_TYPE
        // std::vector<D3D_FEATURE_LEVEL>
        // DebugLayer (bool)
        // BufferCount
        // MultiSample
        // DXGI_FORMAT (BufferFormat)
        class DeviceAsset : public JasonMetaDataAsset<D3D_DRIVER_TYPE, std::vector<D3D_FEATURE_LEVEL>, bool, uint32_t, uint32_t, DXGI_FORMAT>
        {
        public:
            DeviceAsset(const yaget::io::Tag& tag, yaget::io::Buffer buffer, const yaget::io::VirtualTransportSystem& vts)
                : JasonMetaDataAsset(tag, buffer, vts, "Device",
                    []() { return true; },
                    [this]() { return json::GetValue<D3D_DRIVER_TYPE>(root, "DeviceType", D3D_DRIVER_TYPE_HARDWARE); },
                    [this]() { return json::GetValue<std::vector<D3D_FEATURE_LEVEL>>(root, "FeatureLevels", {}); },
                    [this]() { return json::GetValue<bool>(root, "DebugLayer", false); },
                    [this]() { return json::GetValue<uint32_t>(root, "BufferCount", 2); },
                    [this]() { return json::GetValue<uint32_t>(root, "MultiSample", 1); },
                    [this]() { return json::GetValue<DXGI_FORMAT>(root, "BufferFormat", DXGI_FORMAT_R8G8B8A8_UNORM); })
                    , mType(std::get<0>(mFields))
                    , mFeatureLevels(std::get<1>(mFields))
                    , mDebugLayer(std::get<2>(mFields))
                    , mBufferCount(std::get<3>(mFields))
                    , mMultiSample(std::get<4>(mFields))
                    , mBufferFormat(std::get<5>(mFields))
            {}

            const D3D_DRIVER_TYPE& mType;
            const std::vector<D3D_FEATURE_LEVEL>& mFeatureLevels;
            const bool& mDebugLayer;
            const uint32_t& mBufferCount;
            const uint32_t& mMultiSample;
            const DXGI_FORMAT& mBufferFormat;
        };

    } // namespace io::render

    namespace conv
    {
        //-------------------------------------------------------------------------------------------------------------------------------
        template <>
        struct Convertor<D3D_DRIVER_TYPE>
        {
            static std::string ToString(const D3D_DRIVER_TYPE& value)
            {
                return fmt::format("D3D_DRIVER_TYPE: '{}'", value);
            }
        };

        //-------------------------------------------------------------------------------------------------------------------------------
        template <>
        struct Convertor<std::vector<D3D_FEATURE_LEVEL>>
        {
            static std::string ToString(const std::vector<D3D_FEATURE_LEVEL>& value)
            {
                std::string result = fmt::format("D3D_FEATURE_LEVELS: '{}' [", value.size());
                std::string delim;
                for (const auto& v : value)
                {
                    result += delim;
                    result += fmt::format("'{}'", v);
                    delim = ", ";
                }

                result += "]";

                return result;
            }
        };

    } // namespace conv
} // namespace yaget


NLOHMANN_JSON_SERIALIZE_ENUM(D3D_DRIVER_TYPE, {
    { D3D_DRIVER_TYPE_HARDWARE,         nullptr },
    { D3D_DRIVER_TYPE_HARDWARE,         "Hardware" },
    { D3D_DRIVER_TYPE_REFERENCE,        "Reference" },
    { D3D_DRIVER_TYPE_NULL,             "Null" }
});

NLOHMANN_JSON_SERIALIZE_ENUM(D3D_FEATURE_LEVEL, {
    { D3D_FEATURE_LEVEL_11_0,           nullptr },
    { D3D_FEATURE_LEVEL_9_1,            "9_1" },
    { D3D_FEATURE_LEVEL_9_2,            "9_2" },
    { D3D_FEATURE_LEVEL_10_0,           "10_0" },
    { D3D_FEATURE_LEVEL_10_1,           "10_1" },
    { D3D_FEATURE_LEVEL_11_0,           "11_0" },
    { D3D_FEATURE_LEVEL_11_1,           "11_1" },
    { D3D_FEATURE_LEVEL_12_0,           "12_0" },
    { D3D_FEATURE_LEVEL_12_1,           "12_1" }
});

NLOHMANN_JSON_SERIALIZE_ENUM(DXGI_FORMAT, {
    { DXGI_FORMAT_R8G8B8A8_UNORM,       nullptr },
    { DXGI_FORMAT_R8G8B8A8_UNORM,       "R8G8B8A8_UNORM" },
    { DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,  "R8G8B8A8_UNORM_SRGB" }
});


