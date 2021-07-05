/////////////////////////////////////////////////////////////////////////
// Device.h
//
//  Copyright June 19 2021 Edgar Glowacki.
//
// NOTES:
//      Another attempt at dx12 rendering
//
//      Create Window
//      Create Device
//      Create Command Queue
//      Create Swap Chain
//      Create Back Buffers
//      Create Root Signature
//      Create Pipeline State Object
//      Create Command Lists
//      Create Vertex Buffers and etc.
//      Loop
//          Populate Command Lists.
//          Execute Command Lists.
//          Wait(fence)
//          Display a beautifully rendered frame.
//          Reset the Command Listsand Allocators.
//      Begin clean up by using Wait(fence).
//      Release all D3D12 objects.
//      https://vzout.com/c++/directx12_tutorial.html
//
//      https://www.3dgep.com/learning-directx-12-2/
//
// #include "Render/Device.h"
//
/////////////////////////////////////////////////////////////////////////
//! \file

#pragma once

#include "Render/RenderCore.h"
#include "App/WindowFrame.h"
#include "Render/Waiter.h"

//#include <d3dx12.h>

namespace yaget
{
    namespace metrics { class Channel; }
    namespace time { class GameClock; }
}


namespace yaget::render
{
    namespace platform
    {
        class Adapter;
        class Fence;
        class SwapChain;
    }

    class DeviceB : public Noncopyable<DeviceB>
    {
    public:
        DeviceB(app::WindowFrame windowFrame);
        ~DeviceB();

        void Resize();
        void SurfaceStateChange();
        int64_t OnHandleRawInput(void* hWnd, uint32_t message, uint64_t wParam, int64_t lParam);

        void RenderFrame(const time::GameClock& gameClock, metrics::Channel& channel);

    private:
        app::WindowFrame mWindowFrame;
        Waiter mWaiter;

        std::unique_ptr<platform::Adapter> mAdapter;
        std::unique_ptr<platform::SwapChain> mSwapChain;
    };

}
