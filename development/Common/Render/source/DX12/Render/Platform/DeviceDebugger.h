/////////////////////////////////////////////////////////////////////////
// DeviceDebugger.h
//
//  Copyright 06/12/2021 Edgar Glowacki.
//
// NOTES:
//      Deals with debug layer for platform device
//
// #include "DeviceDebugger.h"
//
/////////////////////////////////////////////////////////////////////////
//! \file

#pragma once

#include "YagetCore.h"
#include <wrl/client.h>

struct ID3D12Debug1;

namespace yaget::render::platform
{
    // When this object is created, it will require to use
    // DXGI_CREATE_FACTORY_DEBUG flag in CreateDXGIFactory2(...) call
    class DeviceDebugger
    {
    public:
        DeviceDebugger();
        ~DeviceDebugger() = default;

    private:
        Microsoft::WRL::ComPtr<ID3D12Debug1> mDebugController;
    };
}
