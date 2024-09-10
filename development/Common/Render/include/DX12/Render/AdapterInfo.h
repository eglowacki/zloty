/////////////////////////////////////////////////////////////////////////
// AdapterInfo.h
//
//  Copyright 06/12/2021 Edgar Glowacki.
//
// NOTES:
//      Deals with adapter which will create Device
//
// #include "Render/AdapterInfo.h"
//
/////////////////////////////////////////////////////////////////////////
//! \file

#pragma once

#include "Render/RenderCore.h"
#include <functional>

struct ID3D12Device;
struct IDXGIAdapter;
struct IDXGIFactory;

namespace D3D12MA { class Allocator; }

// Doing this here so I do not have to include dx12 header files. Those values are enums in those
// header files and we simply use int type.
using D3D_FEATURE_LEVEL_Y = int;
using DXGI_FORMAT_Y = int;
constexpr D3D_FEATURE_LEVEL_Y D3D_FEATURE_LEVEL_1_0_CORE_Y = 0x1000;

//--------------------------------------------------------------------------------
// data structures representing various video and device settings
namespace yaget::render::info
{
    struct Resolution
    {
        size_t mWidth = 0;
        size_t mHeight = 0;
        long mRefreshRate = 0;
        DXGI_FORMAT_Y mFormat = 0;

        bool IsValid() const
        {
            return mWidth && mHeight && mFormat != 0;
        }
    };

    struct Output
    {
        std::string mName;
        HMONITOR mMonitor{};

        std::vector<Resolution> mResolutions;

        bool IsValid() const
        {
            return !mName.empty() && 
                mMonitor &&
                !mResolutions.empty() &&
                mResolutions.begin()->IsValid();
        }
    };

    struct Adapter
    {
        std::string mName;
        bool mSoftware = false;
        uint64_t mVideoMemory = 0;
        LUID mId{};
        D3D_FEATURE_LEVEL_Y mFeatureLevel = D3D_FEATURE_LEVEL_1_0_CORE_Y;

        std::vector<Output> mOutputs;

        // Return true if Adapter data is valid, has at least one Output and one Resolution.
        bool IsValid() const
        {
            return !mName.empty() && 
                !mOutputs.empty() &&
                mOutputs.begin()->IsValid();
        }

        const Resolution& GetSelectedResolution() const
        {
            YAGET_ASSERT(IsValid(), "Adapter is not valid");

            return *mOutputs.begin()->mResolutions.begin();
        }
    };

    using Adapters = std::vector<Adapter>;

    // used to filter which display info to collect.
    // When filter returns true, keep that, otherwise skip.
    // If filter is not set for, it is treated as true.
    struct Filters
    {
        // convenience functions
        bool IsFeatureLevel(D3D_FEATURE_LEVEL_Y featureLevel) const { return mFeatureLevel ? mFeatureLevel(featureLevel) : true; }
        bool IsAdapter(const std::string& name) const { return mAdapter ? mAdapter(name) : true; }
        bool IsOutput(const std::string& name) const { return mOutput ? mOutput(name) : true; }
        bool IsFormat(DXGI_FORMAT_Y format) const { return mFormat ? mFormat(format) : true; }
        bool IsResolution(const Resolution& resolution) const { return mResolution ? mResolution(resolution) : true; }

        std::function<bool(D3D_FEATURE_LEVEL_Y /*featureLevel*/)> mFeatureLevel;
        std::function<bool(const std::string& /*name*/)> mAdapter;
        std::function<bool(const std::string& /*name*/)> mOutput;
        std::function<bool(DXGI_FORMAT_Y /*format*/)> mFormat;
        std::function<bool(const Resolution& /*resolution*/)> mResolution;
    };

    // Construct filters that provides DX12+ version, 32 bit RGBA and refresh rate of 60+
    Filters GetDefaultFilters();

    // get all valid adapters, it outputs and all resolutions for each output
    Adapters EnumerateAdapters(Filters filters, bool referenceRasterizer);
    // return one specific Adapter to be used for CreateDevice
    Adapter SelectAdapter(const Adapters& adapters, Filters filters);
    // return adapter representing resolution or default adapter if no resolution supported
    Adapter SelectDefaultAdapter(size_t configInitBlock_ResX, size_t configInitBlock_ResY);

    // This get's returned by CreateDevice(...) function. It provides all three DX12 objects
    // representing device, adapter and factory.
    using HardwareDevice = std::tuple<ComPtr<ID3D12Device>, ComPtr<IDXGIAdapter>, ComPtr<IDXGIFactory>>;
    // Create actual device and initialize (debug is on in non-shipping configurations)
    HardwareDevice CreateDevice(const Adapter& adapter);

} // namespace yaget::render::info
