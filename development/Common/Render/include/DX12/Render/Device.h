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
#include "MathFacade.h"


namespace yaget
{
    namespace metrics { class Channel; }
    namespace time { class GameClock; }
}


namespace yaget::render
{
    class Polygon;
    namespace platform
    {
        class Adapter;
        class CommandAllocators;
        class CommandQueues;
        class CommandListPool;
        class Fence;
        class SwapChain;
    }
    namespace info
    {
        struct Adapter;
    }


    //-------------------------------------------------------------------------------------------------
    class ColorInterpolator
    {
    public:
        ColorInterpolator(const colors::Color& startColor, const colors::Color& endColor);

        colors::Color GetColor(const time::GameClock& gameClock);

    private:
        const colors::Color mStartColor = colors::White;
        const colors::Color mEndColor = colors::Black;
        float mCurrentColorT = 0.0f;
        float mColorTDirection = 1.0f;
    };


    //-------------------------------------------------------------------------------------------------
    class DeviceB : public Noncopyable<DeviceB>
    {
    public:
        DeviceB(app::WindowFrame windowFrame, const yaget::render::info::Adapter& adapterInfo);
        ~DeviceB();

        void Resize();
        void SurfaceStateChange();
        int64_t OnHandleRawInput(app::DisplaySurface::PlatformWindowHandle hWnd, uint32_t message, uint64_t wParam, int64_t lParam);

        void RenderFrame(const time::GameClock& gameClock, metrics::Channel& channel);

    private:
        app::WindowFrame mWindowFrame;
        Waiter mWaiter;

        std::unique_ptr<platform::Adapter> mAdapter;
        std::unique_ptr<Polygon> mPolygon;
        std::unique_ptr<Polygon> mPolygon2;
        std::unique_ptr<platform::CommandAllocators> mCommandAllocators;
        std::unique_ptr<platform::CommandQueues> mCommandQueues;
        std::unique_ptr<platform::SwapChain> mSwapChain;
        std::unique_ptr<platform::CommandListPool> mCommandListPool;

        ColorInterpolator mColorInterpolator;
    };

}
