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
        Application& mApplication;
        std::unique_ptr<platform::HardwareDevice> mHardwareDevice;
    };

}
