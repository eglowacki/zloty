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
//  #include "DesktopApplication.h"
//
//////////////////////////////////////////////////////////////////////
//! \file
#pragma once

#include "YagetCore.h"
#include "App/WindowApplication.h"
#include "Device.h"
#include "Gui/Support.h"


namespace yaget
{
    namespace render
    {
        class DesktopApplication : public WindowApplication
        {
        public:
            DesktopApplication(const std::string& title, items::Director& director, io::VirtualTransportSystem& vts, const args::Options& options, const Device::TagResourceResolvers& tagResolvers)
                : WindowApplication(title, director, vts, options)
                , mDevice(*this, tagResolvers, mWatcher)
            {
                if (Input().IsAction("Quit App"))
                {
                    Input().RegisterSimpleActionCallback("Quit App", [this]() { RequestQuit(); });
                }

                if (Input().IsAction("Konsole"))
                {
                    Input().RegisterSimpleActionCallback("Konsole", []() { dev::CurrentConfiguration().mDebug.RefreshGui(!dev::CurrentConfiguration().mDebug.mFlags.Gui); });
                }

                if (Input().IsAction("FullScreenToggle"))
                {
                    // NOTE: what we need here is to send message to main thread that we request surface change, which in turn will actually initiate
                    // the change request from main thread, which in turn will resize surface, call DXGDI, etc
                    Input().RegisterSimpleActionCallback("FullScreenToggle", [This = this]()
                    {
                        This->AddEvent([This]()
                        {
                            This->mDevice.SurfaceStateChange();
                        });
                    });
                }
            }

            void OnResize() override  { mDevice.Resize(); }
            io::Watcher& Watcher() { return mWatcher; }
            Device& GetDevice() { return mDevice; }

        private:
            int64_t onHandleRawInput(WindowHandle_t hWnd, uint32_t message, uint64_t wParam, int64_t lParam) override
            {
                return yaget::gui::ProcessInput(hWnd, message, wParam, lParam);
            }

            void OnSurfaceStateChange() override { mDevice.SurfaceStateChange(); }

            io::Watcher mWatcher;
            Device mDevice;
        };

    } // namespace render
} // namespace yaget
