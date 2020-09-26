//////////////////////////////////////////////////////////////////////
// PlatformDisplayModes.h
//
//  Copyright 9/7/2019 Edgar Glowacki
//
//  Maintained by: Edgar
//
//  NOTES:
//      Helpers for querying display modes
//
//
//  #include "PlatformDisplayModes.h"
//
//////////////////////////////////////////////////////////////////////
//! \file
#pragma once

#include "YagetCore.h"
#include <wrl/client.h>
#include <dxgiformat.h>

struct IDXGIAdapter;

namespace yaget
{
    namespace render::platform
    {
        // Represents
        struct Adapter
        {
            struct Resolutions
            {
                uint32_t Width = 0;
                uint32_t Height = 0;
                uint32_t RefreshRate = 0;
            };

            Microsoft::WRL::ComPtr<IDXGIAdapter> mPlatfrormAdapter;
            HMONITOR mMonitor = nullptr;
            DXGI_FORMAT mColorFormat;
            std::string AdapterName;
            std::string MonitorName;
            uint32_t Width = 0;
            uint32_t Height = 0;
            std::vector<Resolutions> Modes;
        };
        using Adapters = std::vector<Adapter>;
        using Resolutions = std::map<DXGI_FORMAT, Adapters>;

        Adapters GetResolutions(DXGI_FORMAT format);
        Resolutions GetResolutions();

    } // namespace render::platform
} // namespace yaget
