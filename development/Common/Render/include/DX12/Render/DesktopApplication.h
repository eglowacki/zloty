//////////////////////////////////////////////////////////////////////
// DesktopApplication.h
//
//  Copyright 6/29/2019 Edgar Glowacki
//
//  Maintained by: Edgar
//
//  NOTES:
//      Helper class for basic windows desktop application
//      and device for rendering
//
//
//  #include "RenderDesktopApplication.h"
//
//////////////////////////////////////////////////////////////////////
//! \file
#pragma once

#include "YagetCore.h"
#include "App/WindowApplication.h"
#include "Render/Device.h"


namespace yaget::render
{
    class DesktopApplication : public WindowApplication
    {
    public:
        DesktopApplication(const std::string& title, items::Director& director, io::VirtualTransportSystem& vts, const args::Options& options)
            : WindowApplication(title, director, vts, options)
            , mDevice(*this)
        {
            if (Input().IsAction("Quit App"))
            {
                Input().RegisterSimpleActionCallback("Quit App", [this]() { RequestQuit(); });
            }
        }

        void OnResize() override { mDevice.Resize(); }
        Device& GetDevice() { return mDevice; }

    private:
        int64_t onHandleRawInput(WindowHandle_t /*hWnd*/, uint32_t /*message*/, uint64_t /*wParam*/, int64_t /*lParam*/) override { return 0; }
        void OnSurfaceStateChange() override { mDevice.SurfaceStateChange(); }

        Device mDevice;
    };

}
