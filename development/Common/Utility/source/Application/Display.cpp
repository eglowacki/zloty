#include "App/Display.h"
#include "Debugging/DevConfiguration.h"

namespace 
{
    //-------------------------------------------------------------------------------------------------
    math3d::Vector2 GetWindowSize(yaget::app::DisplaySurface::PlatformWindowHandle handle)
    {
        if (!handle)
        {
            return math3d::Vector2(1, 1);
        }

        RECT clientRect;
        ::GetClientRect(reinterpret_cast<HWND>(handle), &clientRect);

        const auto w = std::max<int>(1, clientRect.right - clientRect.left);
        const auto h = std::max<int>(1, clientRect.bottom - clientRect.top);
        return { static_cast<float>(w), static_cast<float>(h) };
    }

    //--------------------------------------------------------------------------------------------------
    BOOL CALLBACK MonitorEnumProc(__in  HMONITOR hMonitor, __in  HDC hdcMonitor, __in  LPRECT lprcMonitor, __in  LPARAM dwData)
    {
        hdcMonitor;
        lprcMonitor;

        std::vector<yaget::app::MonitorInfoEx>& infoArray = *reinterpret_cast<std::vector<yaget::app::MonitorInfoEx>*>(dwData);
        yaget::app::MonitorInfoEx info;
        ::GetMonitorInfo(hMonitor, &info);
        info.hMonitor = hMonitor;
        info.mIndex = static_cast<int>(infoArray.size() + 1);
        infoArray.push_back(info);

        return TRUE;
    }

    //-------------------------------------------------------------------------------------------------
    yaget::app::SysDisplays::Monitors GetAllMonitors()
    {
        yaget::app::SysDisplays::Monitors info;
        info.reserve(::GetSystemMetrics(SM_CMONITORS));
        ::EnumDisplayMonitors(nullptr, nullptr, MonitorEnumProc, reinterpret_cast<LPARAM>(&info));

        return info;
    }

}


//-------------------------------------------------------------------------------------------------
yaget::app::DisplaySurface::DisplaySurface(PlatformWindowHandle handle, SurfaceState surfaceState)
: mHandle(handle)
, mSize(GetWindowSize(handle))
, mSurfaceState(surfaceState)
{
}


//-------------------------------------------------------------------------------------------------
bool yaget::app::DisplaySurface::VSync() const
{
    return true;
}


//-------------------------------------------------------------------------------------------------
int yaget::app::DisplaySurface::NumBackBuffers() const
{
    return dev::CurrentConfiguration().mInit.NumBackBuffers;
}


//-------------------------------------------------------------------------------------------------
yaget::app::SysDisplays::SysDisplays()
    : mInfo(GetAllMonitors())
{
    
}


//-------------------------------------------------------------------------------------------------
bool yaget::app::SysDisplays::Intersect(const RECT& windowPos) const
{
    for (const auto &monitor : mInfo)
    {
        RECT dummyDest;
        if (IntersectRect(&dummyDest, monitor.GetWorkRect(), &windowPos))
        {
            return true;
        }
    }

    return false;
}


//-------------------------------------------------------------------------------------------------
const yaget::app::MonitorInfoEx& yaget::app::SysDisplays::FindPrimary() const
{
    auto it = std::find_if(std::begin(mInfo), std::end(mInfo), [](const auto& monitor)
    {
        return monitor.IsPrimary();
    });

    YAGET_ASSERT(it != std::end(mInfo), "There is no primary monitor set on this system.");

    return *it;
}


//-------------------------------------------------------------------------------------------------
const yaget::app::MonitorInfoEx& yaget::app::SysDisplays::Find(int index) const
{
    auto it = std::find_if(std::begin(mInfo), std::end(mInfo), [index](const auto& monitor)
    {
        return monitor.mIndex == index;
    });

    if (it != std::end(mInfo))
    {
        return *it;
    }

    return FindPrimary();
}


//-------------------------------------------------------------------------------------------------
const yaget::app::MonitorInfoEx& yaget::app::SysDisplays::FindNearest(HWND hWnd) const
{
    HMONITOR windowMonitor = ::MonitorFromWindow(hWnd, MONITOR_DEFAULTTONEAREST);

    for (const auto &monitor : mInfo)
    {
        if (monitor.hMonitor == windowMonitor)
        {
            return monitor;
        }
    }

    return FindPrimary();
}

