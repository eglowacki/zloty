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
#ifndef YAGET_WINDOW_APLICATION_H
#define YAGET_WINDOW_APLICATION_H

#include "Application.h"

namespace yaget
{
    namespace io { class VirtualTransportSystem; }

    class WindowApplication : public Application
    {
    public:
        WindowApplication(const std::string& title, items::Director& director, io::VirtualTransportSystem& vts, const args::Options& options);
        virtual ~WindowApplication();

        math3d::Vector2 GetWindowSize() const override;


        // used internally
        void _onSuspend(bool bSuspend);
        void _onResise();
        int _processInputMessage(int64_t lParam);
        int _processMouseMessage(uint32_t message, uint64_t wParam, int64_t lParam);
        int64_t _onHandleInputMessage(WindowHandle_t hWnd, uint32_t message, uint64_t wParam, int64_t lParam);
        void _onToggleFullScreen();

    private:
        virtual void OnResize() = 0;
        bool onMessagePump(const time::GameClock& gameClock) override;
        virtual int64_t onHandleRawInput(WindowHandle_t hWnd, uint32_t message, uint64_t wParam, int64_t lParam) = 0;
        void Cleanup() override;

        uint32_t mLastKeyFlags = 0;

        // handling of window to cover entire screen
        bool mFullScreen = false;
        uint32_t mWindowStyle = 0;
        uint32_t mWindowExStyle = 0;
        int32_t mWindowLeft = 0;
        int32_t mWindowTop = 0;
        int32_t mWindowRight = 0;
        int32_t mWindowBottom = 0;
    };
} // namespace yaget

#endif // YAGET_WINDOW_APLICATION_H
