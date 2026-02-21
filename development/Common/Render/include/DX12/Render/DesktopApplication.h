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
#include "Components/GameSystem.h"
#include "Render/Device.h"


namespace yaget::render
{
    class DesktopApplication : public WindowApplication
    {
    public:
        DesktopApplication(const std::string& title, items::Director& director, io::VirtualTransportSystem& vts, const args::Options& options, const yaget::render::info::Adapter& selectedAdapter)
            : WindowApplication(title, director, vts, options)
            , mDevice(app::WindowFrame(*this), selectedAdapter)
        {
            if (Input().IsAction("Quit App"))
            {
                Input().RegisterSimpleActionCallback("Quit App", [this]() { RequestQuit(); });
            }
        }

        DeviceB& Device() { return mDevice; }
        const DeviceB& Device() const { return mDevice; }

    private:
        void OnResize() override { Device().Resize(); }
        void OnSurfaceStateChange() override { Device().SurfaceStateChange(); }
        int64_t onHandleRawInput(WindowHandle_t hWnd, uint32_t message, uint64_t wParam, int64_t lParam) override
        {
            return Device().OnHandleRawInput(hWnd, message, wParam, lParam);
        }

        DeviceB mDevice;
    };

    template<typename CS, typename EndMarker, typename M, typename... Comp>
    class RenderSystemApp : public comp::gs::GameSystem<CS, EndMarker, M, Comp...>
    {
    protected:
        RenderSystemApp(const char* niceName, M& messaging, Application& app, comp::gs::GameSystem<CS, EndMarker, M, Comp...>::UpdateFunctor updateFunctor, CS& coordinatorSet, bool tickEnabled)
            : comp::gs::GameSystem<CS, EndMarker, M, Comp...>(niceName, messaging, app, updateFunctor, coordinatorSet, tickEnabled)
            , mDevice(static_cast<DesktopApplication&>(app).Device())
        {}

        DeviceB& GetDevice() const
        {
            return mDevice;
        }

    private:
        DeviceB& mDevice;
    };

}
