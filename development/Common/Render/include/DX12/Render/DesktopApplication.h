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
//  #include "Render/DesktopApplication.h"
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
        DesktopApplication(const std::string& title, items::Director& director, io::VirtualTransportSystem& vts, const args::Options& options, const yaget::render::info::Adapter& selectedAdapter);
        DeviceB& GetDevice() { return mDevice; }

    private:
        void OnResize() override { mDevice.Resize(); }
        void OnSurfaceStateChange() override { mDevice.SurfaceStateChange(); }
        int64_t onHandleRawInput(WindowHandle_t hWnd, uint32_t message, uint64_t wParam, int64_t lParam) override
        {
            return mDevice.OnHandleRawInput(hWnd, message, wParam, lParam);
        }

        DeviceB mDevice;
    };

}
