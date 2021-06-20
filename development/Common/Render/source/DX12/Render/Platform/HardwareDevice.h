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

#if 0
struct ID3D12Device2;
struct ID3D12Debug1;
struct ID3D12CommandQueue;

namespace yaget
{
    namespace time { class GameClock; }
    namespace metrics { class Channel; }
    class Application;
}

namespace yaget::render::platform
{
    class HardwareDevice
    {
    public:
        HardwareDevice(Application& app);
        ~HardwareDevice();

        void Render(const time::GameClock& gameClock, metrics::Channel& channel);
        void Resize();

    private:
#ifdef YAGET_DEBUG
        Microsoft::WRL::ComPtr<ID3D12Debug1> mDebugDevice;
#endif // YAGET_DEBUG

        Microsoft::WRL::ComPtr<ID3D12Device2> mDevice;
        SwapChain mSwapChain;
    };

} // yaget::render::platform
#endif