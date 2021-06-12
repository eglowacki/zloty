//////////////////////////////////////////////////////////////////////
// Device.h
//
//  Copyright 06/06/2021 Edgar Glowacki
//
//  Maintained by: Edgar
//
//  NOTES:
//      
//
//
//  #include "Render/Device.h"
//
//////////////////////////////////////////////////////////////////////
//! \file
#pragma once

#include "YagetCore.h"

namespace yaget
{
    namespace metrics { class Channel; }
    namespace time { class GameClock; }

    class Application;
}

namespace yaget::render
{
    namespace platform { class HardwareDevice; }

    class Device : public Noncopyable<Device>
    {
    public:
        Device(Application& app);
        ~Device();

        void Resize();
        void SurfaceStateChange();

        void RenderFrame(const time::GameClock& gameClock, metrics::Channel& channel);

    private:
        struct Waiter
        {
            void Wait();

            void BeginPause();
            void EndPause();

            std::mutex mPauseRenderMutex;
            std::condition_variable mWaitForRenderThread;
            std::atomic_bool mPauseCounter{ false };
            std::condition_variable mRenderPaused;

            int mUsageCounter = 0;
        };

        struct WaiterScoper
        {
            WaiterScoper(Waiter& waiter) : waiter(waiter)
            {
                waiter.BeginPause();
            }

            ~WaiterScoper()
            {
                waiter.EndPause();
            }

            Waiter& waiter;
        };


        Application& mApplication;
        std::unique_ptr<platform::HardwareDevice> mHardwareDevice;
        Waiter mWaiter;
    };

}
