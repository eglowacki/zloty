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

#include "Display.h"
#include "IdGameCache.h"
#include "Input/InputDevice.h"
#include "Metrics/Concurrency.h"
#include "ThreadModel/JobPool.h"
#include "Time/GameClock.h"

#include <functional>


namespace yaget
{
    namespace io { class VirtualTransportSystem; }
    namespace items { class Director; }

    class Application : public NonCopyMove<Application>  // NOLINT(cppcoreguidelines-special-member-functions)
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

        using WindowHandle_t = app::DisplaySurface::PlatformWindowHandle;

        virtual ~Application();

        using TickLogic = std::function<void(const time::GameClock&, metrics::Channel&)>;
        using TickRender = TickLogic;
        using TickIdle = std::function<void()>;
        using QuitLogic = TickLogic;
        using QuitApplication = std::function<void()>;
        int Run(const TickLogic& tickLogic, const TickRender& tickRender = {}, const TickIdle& tickIdle = {}, const QuitLogic& shutdownLogicCallback = {}, const QuitApplication& quitCallback = {});

        void RequestQuit();
        input::InputDevice& Input() { return mInputDevice; }

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
        std::atomic_bool mQuit{false};
        bool mLastKeyState[256] = {false};
        time::GameClock mApplicationClock;
        time::GameClock mRenderClock;
        input::InputDevice mInputDevice;
        std::unique_ptr<Mouse> mLastMouseInput{};

    private:
        virtual bool IsSuspended() const = 0;
        virtual bool onMessagePump(const time::GameClock& gameClock) = 0;
        virtual void Cleanup() = 0;
        void onRenderTask(const TickLogic& renderCallback);
        void onLogicTask(const TickLogic& logicCallback, const TickLogic& shutdownLogicCallback);

        // when user request quit, this will be processed on the next frame, to make sure orderly exit
        std::atomic_bool mRequestQuit{false};
        std::unique_ptr<mt::JobPool> mGeneralPoolThread{};
        using State_t = double;
        State_t mRenderState = 0.0;
        io::VirtualTransportSystem& mVTS;
    };
} // namespace yaget
