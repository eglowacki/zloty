/////////////////////////////////////////////////////////////////////////
// HardwareDevice.h
//
//  Copyright HardwareDevice.h Edgar Glowacki.
//
// NOTES:
//      
//
// #include "Render/Platform/HardwareDevice.h"
//
/////////////////////////////////////////////////////////////////////////
//! \file

#pragma once


#include "YagetCore.h"
#include "Render/Platform/SwapChain.h"


struct ID3D12Device2;
struct ID3D12DebugDevice;
struct ID3D12CommandQueue;

namespace yaget {
    class Application;
}

namespace yaget::render::platform
{
    class HardwareDevice
    {
    public:
        HardwareDevice(Application& app);
        ~HardwareDevice();

        void Render();
        void Resize();

    private:
        Microsoft::WRL::ComPtr<ID3D12Device2> mDevice;
        SwapChain mSwapChain;

#ifdef YAGET_DEBUG
        Microsoft::WRL::ComPtr<ID3D12DebugDevice> mDebugDevice;
#endif // YAGET_DEBUG
    };

} // yaget::render::platform
