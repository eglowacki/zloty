//////////////////////////////////////////////////////////////////////
// Application.h
//
//  Copyright 7/22/2016 Edgar Glowacki.
//
//  Maintained by: Edgar
//
//  NOTES:
//      Provides base class to derive for app window and message pump
//
//
//  #include "App/Application.h"
//
//////////////////////////////////////////////////////////////////////
//! \file
#pragma once

#include "Time/GameClock.h"
#include "Input/InputDevice.h"
#include "Metrics/Concurrency.h"
#include "IdGameCache.h"
#include "ThreadModel/JobPool.h"
#include "Display.h"

#include <functional>



namespace yaget
{
    namespace io { class VirtualTransportSystem; }
    namespace items { class Director; }

    class Application : public Noncopyable<Application>
    {

    public:
        enum class ReturnCode
        {
            OK = 0,
            ERROR_INIT = 1,
            ERROR_OPTIONS = 2,
            ERROR_LOGIC = 3
        };

        static int ExitCode(ReturnCode returnCode) { return static_cast<int>(returnCode); }

        using WindowHandle_t = void*;

        virtual ~Application() = default;

        // DEPRECATED block of Run callbacks
        using StatusCallback_t = std::function<void()>;
        using UpdateCallback_t = std::function<void(Application&, const time::GameClock&, metrics::Channel&)>;
        int Run(const UpdateCallback_t& logicCallback, const UpdateCallback_t& shutdownLogicCallback, const UpdateCallback_t& renderCallback, const StatusCallback_t& idleCallback, const StatusCallback_t& quitCallback);

        using TickLogic = std::function<void(const time::GameClock&, metrics::Channel&)>;
        using TickRender = TickLogic;
        using TickIdle = std::function<void()>;
        int Run(const TickLogic& tickLogic, const TickRender& tickRender = nullptr, const TickIdle& tickIdle = nullptr);

        void RequestQuit();
        input::InputDevice& Input() {return mInputDevice;}

        const args::Options Options;
        IdGameCache& IdCache;
        io::VirtualTransportSystem& VTS() { return mVTS; }
        const io::VirtualTransportSystem& VTS() const { return mVTS; }
        items::Director& Director() { return mDirector; }
        const items::Director& Director() const { return mDirector; }

        struct VideoOptions
        {
            uint32_t mMonitorId;
            uint32_t mDisplayMode;
            uint32_t mResX;
            uint32_t mResY;
            bool mSync;
            bool mFullScreen;
        };
        // Request mode change. The mode may or may not succeed, or simply not look well on user system
        // it is up to caller to provide some kind of confirmation of video options change
        void ChangeVideoSettings(const VideoOptions& videoOptions);

        virtual app::DisplaySurface GetSurface() const = 0;

    protected:
        Application(const std::string& title, items::Director& director, io::VirtualTransportSystem& vts, const args::Options& options);

        using Mouse = input::InputDevice::Mouse;

        items::Director& mDirector;
        std::atomic_bool mQuit{ false };
        bool mLastKeyState[256] = { false };
        time::GameClock mApplicationClock;
        input::InputDevice mInputDevice;
        std::unique_ptr<Mouse> mLastMouseInput;

    private:
        virtual bool onMessagePump(const time::GameClock& gameClock) = 0;
        virtual void Cleanup() = 0;
        void onRenderTask(const UpdateCallback_t& renderCallback);
        void onLogicTask(const UpdateCallback_t& logicCallback, const UpdateCallback_t& shutdownLogicCallback);

        // when user request quit, this will be processed on the next frame, to make sure orderly exit
        std::atomic_bool mRequestQuit{ false };
        std::unique_ptr<mt::JobPool> mGeneralPoolThread;
        using State_t = double;
        State_t mRenderState = 0.0;
        io::VirtualTransportSystem& mVTS;
    };

} // namespace yaget


