/////////////////////////////////////////////////////////////////////////
// Display.h
//
//  Copyright 4/11/2009 Edgar Glowacki.
//
// NOTES:
//      Exposes display surface and data for rendering to it
//
//
// #include "App/Display.h"
//
/////////////////////////////////////////////////////////////////////////
//! \file
#pragma once

#include "YagetCore.h"
#include "MathFacade.h"

namespace yaget::app
{
    //--------------------------------------------------------------------------------------------------
    // Represents what type of surface rendering device will use, exclusive (full screen), shared (window, borderless)
    enum class SurfaceState { Shared, Exclusive };

    //--------------------------------------------------------------------------------------------------
    class DisplaySurface
    {
    public:
        using PlatformWindowHandle = void*;

        DisplaySurface(PlatformWindowHandle handle, SurfaceState surfaceState);

        const math3d::Vector2 Size() const;
        SurfaceState State() const { return mSurfaceState; }
        bool VSync() const;
        int NumBackBuffers() const;

        // Helper method to cast window handle into specific T
        template <typename T>
        T Handle() const { return reinterpret_cast<T>(mHandle); }

        template <typename T>
        std::tuple<T, T> GetSize() const
        {
            const auto& size = Size();
            const auto width = static_cast<T>(size.x);
            const auto height = static_cast<T>(size.y);

            return { width, height };
        }

    private:
        PlatformWindowHandle mHandle{nullptr};
        SurfaceState mSurfaceState = SurfaceState::Shared;
    };

    //--------------------------------------------------------------------------------------------------
    class MonitorInfoEx : public tagMONITORINFOEXA
    {
    public:
        MonitorInfoEx() : tagMONITORINFOEXA()
        {
            cbSize = sizeof(MONITORINFOEX);
        }

        const RECT* GetRect() const { return &rcMonitor; }
        const RECT* GetWorkRect() const { return &rcWork; }
        std::string DeviceName() const { return szDevice; }

        bool IsPrimary() const { return (dwFlags & MONITORINFOF_PRIMARY) ? true : false; }

        int Width() const { return GetRect()->right - GetRect()->left; }
        int Height() const { return GetRect()->bottom - GetRect()->top; }
        int WorkWidth() const { return GetWorkRect()->right - GetWorkRect()->left; }
        int WorkHeight() const { return GetWorkRect()->bottom - GetWorkRect()->top; }

        HMONITOR hMonitor = nullptr;
        int mIndex = 0;
    };

    //--------------------------------------------------------------------------------------------------
    class SysDisplays
    {
    public:
        using Monitors = std::vector<MonitorInfoEx>;

        SysDisplays();

        bool Intersect(const RECT& windowPos) const;

        const MonitorInfoEx& FindPrimary() const;
        const MonitorInfoEx& FindNearest(HWND hWnd) const;
        const MonitorInfoEx& Find(int index) const;

    private:
        const Monitors mInfo;
    };

}

