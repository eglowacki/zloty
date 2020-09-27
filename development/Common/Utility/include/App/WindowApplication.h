//////////////////////////////////////////////////////////////////////
// WindowApplication.h
//
//  Copyright 7/22/2016 Edgar Glowacki.
//
//  Maintained by: Edgar
//
//  NOTES:
//      Provides main entry point for window application
//
//
//  #include "App/WindowApplication.h"
//
//////////////////////////////////////////////////////////////////////
//! \file
#pragma once

#include "Application.h"


namespace yaget
{
    namespace app { class ProcHandler; }
    namespace io { class VirtualTransportSystem; }

    class WindowApplication : public Application
    {
    public:
        WindowApplication(const std::string& title, items::Director& director, io::VirtualTransportSystem& vts, const args::Options& options);
        virtual ~WindowApplication();

        app::DisplaySurface GetSurface() const override;

    protected:
        using Event = std::function<void()>;

        void AddEvent(Event event);

    private:
        virtual int64_t onHandleRawInput(WindowHandle_t hWnd, uint32_t message, uint64_t wParam, int64_t lParam) = 0;
        virtual void OnResize() = 0;
        virtual void OnSurfaceStateChange() = 0;

        void _onSuspend(bool bSuspend);
        void ProcessResize();
        int _processInputMessage(int64_t lParam);
        int _processMouseMessage(uint32_t message, uint64_t wParam, int64_t lParam);
        int64_t _onHandleInputMessage(WindowHandle_t hWnd, uint32_t message, uint64_t wParam, int64_t lParam);
        int64_t ProcessUserInput(uint32_t message, uint64_t wParam, int64_t lParam);

        bool onMessagePump(const time::GameClock& gameClock) override;
        void Cleanup() override;

        uint32_t mLastKeyFlags = 0;
        std::unique_ptr<app::ProcHandler> mWindowHandler;
        app::SurfaceState mActiveSurfaceState = app::SurfaceState::Shared;

        using RequestedEvents = std::queue<Event>;
        RequestedEvents mRequestedEvents;
        std::mutex mEventsMutex;
    };
} // namespace yaget
