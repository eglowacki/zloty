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
#include "MathFacade.h"
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

        static int ExitCode(ReturnCode returnCode)
        {
            return static_cast<int>(returnCode);
        }

        typedef void* WindowHandle_t;

        // This show state of this window for display purpose.
        enum class Surface { Window, Borderless, Fullscreen, Maximized, Minimized };

        Application(const std::string& title, items::Director& director, io::VirtualTransportSystem& vts, const args::Options& options);
        virtual ~Application();

        using StatusCallback_t = std::function<void()>;
        using UpdateCallback_t = std::function<void(Application&, const time::GameClock&, metrics::Channel&)>;
        int Run(UpdateCallback_t logicCallback, UpdateCallback_t renderCallback, StatusCallback_t idleCallback, StatusCallback_t quitCallback);
        void RequestQuit();
        input::InputDevice& Input() {return mInputDevice;}
        WindowHandle_t GetWindowHandle() const { return mWinHandle; }

        // Helper method to cast window handle into specific T
        template <typename T>
        T GetWindowHandle() const { return reinterpret_cast<T>(mWinHandle); }

        virtual math3d::Vector2 GetWindowSize() const = 0;

        const args::Options Options;
        IdGameCache IdCache;
        io::VirtualTransportSystem& VTS() { return mVTS; }
        const io::VirtualTransportSystem& VTS() const { return mVTS; }
        items::Director& Director() { return mDirector; }
        const items::Director& Director() const { return mDirector; }

        // add generic task to get it done by app pool. If pool does not exist, it will silently drop the task
        void AddTask(const mt::JobProcessor::Task_t& task);

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

    protected:
        using Mouse = input::InputDevice::Mouse;

        items::Director& mDirector;
        std::atomic_bool mQuit{ false };
        WindowHandle_t mWinHandle = nullptr;
        bool mLastKeyState[256] = { false };
        time::GameClock mGameClock;
        input::InputDevice mInputDevice;
        std::unique_ptr<Mouse> mLastMouseInput;
        Surface mSurface = Surface::Window;

    private:
        virtual bool onMessagePump(const time::GameClock& gameClock) = 0;
        virtual void Cleanup() = 0;
        void onRenderTask(UpdateCallback_t renderCallback);
        void onLogicTask(UpdateCallback_t logicCallback);

        // when user request quit, this will be processed on the next frame, to make sure orderly exit
        std::atomic_bool mRequestQuit{ false };
        std::unique_ptr<mt::JobPool> mGeneralPoolThread;
        typedef double State_t;
        State_t mRenderState = 0.0;
        io::VirtualTransportSystem& mVTS;
    };

} // namespace yaget

