#include "App/ProcHandler.h"
#include "App/AppUtilities.h"
#include "App/Display.h"
#include "Debugging/DevConfiguration.h"
#include "VTS/ResolvedAssets.h"

#include <shellscalingapi.h>



namespace 
{
    //--------------------------------------------------------------------------------------------------
    yaget::app::ProcHandler* GetThis(HWND hWnd)
    {
        return reinterpret_cast<yaget::app::ProcHandler *>(::GetWindowLongPtr(hWnd, GWLP_USERDATA));
    }

    //--------------------------------------------------------------------------------------------------
    void UpdateDpiConfiguration(float currentDpi)
    {
        yaget::dev::CurrentConfiguration().mRuntime.RefreshDpi(currentDpi / USER_DEFAULT_SCREEN_DPI);
        YLOG_INFO("WIN", "DPI Scale set at: '%.2f'.", yaget::dev::CurrentConfiguration().mRuntime.DpiScaleFactor);
    }

    //--------------------------------------------------------------------------------------------------
    // return width, height of window which includes all decorations to fit canvas of resX by resY
    std::tuple<int, int> CalculateWindowFrame(HWND hWnd, int resX, int resY)
    {
        RECT rcClient, rcWind;
        POINT ptDiff;
        bool result = ::GetClientRect(hWnd, &rcClient);
        YAGET_UTIL_THROW_ON_RROR(result, "GetClientRect failed.");

        result = ::GetWindowRect(hWnd, &rcWind);
        YAGET_UTIL_THROW_ON_RROR(result, "GetWindowRect failed.");

        ptDiff.x = (rcWind.right - rcWind.left) - rcClient.right;
        ptDiff.y = (rcWind.bottom - rcWind.top) - rcClient.bottom;

        return { resX + ptDiff.x, resY + ptDiff.y };
    }


    constexpr int SafeResolutionX = 800;
    constexpr int SafeResolutionY = 600;

    //--------------------------------------------------------------------------------------------------
    bool UpdateFromCLO(yaget::app::WindowAppearance& windowAppearance, const yaget::dev::Configuration::Init& init)
    {
        using namespace yaget;

        if (init.mCLO.IsResValid())
        {
            windowAppearance.mResX = std::max(SafeResolutionX, init.mCLO.mResolutionX);
            windowAppearance.mResY = std::max(SafeResolutionY, init.mCLO.mResolutionY);
        }

        if (init.mCLO.mFullscreen)
        {
            windowAppearance.mAppearance = app::Appearance::Fullscreen;
        }

        return init.mCLO.IsResValid();
    }

    //--------------------------------------------------------------------------------------------------
    // this will create window of safe default size and center it on primary monitor
    yaget::app::WindowAppearance MakeSafeConfiguration(HWND hWnd, const yaget::dev::Configuration::Init* init)
    {
        using namespace yaget;

        const app::SysDisplays sysDisplays;
        const auto& primary = sysDisplays.FindPrimary();

        app::WindowAppearance windowAppearance{ app::Appearance::Window, SafeResolutionX, SafeResolutionY, primary.mIndex };

        if (init)
        {
            UpdateFromCLO(windowAppearance, *init);
        }

        if (windowAppearance.mAppearance == app::Appearance::Fullscreen)
        {
            if (!init || !init->mCLO.IsResValid())
            {
                windowAppearance.mResX = primary.Width();
                windowAppearance.mResY = primary.Height();
            }
            // i think left needs to be adjusted to the monitor, since the pixel position will be adjusted by where that monitor is located
            // in relationship to other ones
            windowAppearance.mLeft = primary.GetRect()->left;
            windowAppearance.mTop = primary.GetRect()->top;

            windowAppearance.mRight = windowAppearance.mLeft + windowAppearance.mResX;
            windowAppearance.mBottom = windowAppearance.mTop + windowAppearance.mResY;
        }
        else
        {
            // we only do this if we are in window mode, otherwise frameSize will be ResX and ResY
            const auto& frameSize = CalculateWindowFrame(hWnd, windowAppearance.mResX, windowAppearance.mResY);

            windowAppearance.mLeft = (primary.WorkWidth()/2) - (std::get<0>(frameSize)/2);
            windowAppearance.mTop = (primary.WorkHeight()/2) - (std::get<1>(frameSize)/2);

            windowAppearance.mRight = windowAppearance.mLeft + std::get<0>(frameSize);
            windowAppearance.mBottom = windowAppearance.mTop + std::get<1>(frameSize);
        }

        return windowAppearance;
    }

    //--------------------------------------------------------------------------------------------------
    yaget::app::WindowAppearance MakeDefaultConfiguration(HWND hWnd, int resX, int resY)
    {
        yaget::dev::Configuration::Init init;
        init.mCLO.mResolutionX = resX;
        init.mCLO.mResolutionY = resY;
        return MakeSafeConfiguration(hWnd, &init);
    }

    //--------------------------------------------------------------------------------------------------
    //--------------------------------------------------------------------------------------------------
    // these are helper functions to create default appearances for various types
    yaget::app::WindowAppearance MakeWindowDefault(HWND hWnd, int resX, int resY, int monitorIndex)
    {
        using namespace yaget;

        const app::SysDisplays sysDisplays;

        resX = resX <= 0 ? SafeResolutionX : resX;
        resY = resY <= 0 ? SafeResolutionY : resY;
        monitorIndex = monitorIndex == 0 ? sysDisplays.FindPrimary().mIndex : monitorIndex;

        app::WindowAppearance appearance{ app::Appearance::Window, resX, resY, monitorIndex };
        const auto& monitor = sysDisplays.Find(appearance.mMonitorIndex);

        const auto& frameSize = CalculateWindowFrame(hWnd, appearance.mResX, appearance.mResY);

        appearance.mLeft = (monitor.WorkWidth()/2) - (std::get<0>(frameSize)/2);
        appearance.mTop = (monitor.WorkHeight()/2) - (std::get<1>(frameSize)/2);

        appearance.mRight = appearance.mLeft + std::get<0>(frameSize);
        appearance.mBottom = appearance.mTop + std::get<1>(frameSize);

        return appearance;
    }

    //--------------------------------------------------------------------------------------------------
    yaget::app::WindowAppearance MakeBorderlessDefault(HWND /*hWnd*/, int monitorIndex)
    {
        using namespace yaget;

        const app::SysDisplays sysDisplays;

        monitorIndex = monitorIndex == 0 ? sysDisplays.FindPrimary().mIndex : monitorIndex;
        const auto& monitor = sysDisplays.Find(monitorIndex);

        int resX = monitor.Width();
        int resY = monitor.Height();

        yaget::app::WindowAppearance appearance{ yaget::app::Appearance::Borderless, resX, resY, monitorIndex };

        appearance.mLeft = monitor.GetRect()->left;
        appearance.mTop = monitor.GetRect()->top;
        appearance.mRight = appearance.mLeft + appearance.mResX;
        appearance.mBottom = appearance.mTop + appearance.mResY;

        return appearance;
    }

    //--------------------------------------------------------------------------------------------------
    yaget::app::WindowAppearance MakeFullscreenDefault(HWND /*hWnd*/, int resX, int resY, int monitorIndex)
    {
        using namespace yaget;

        const app::SysDisplays sysDisplays;

        monitorIndex = monitorIndex == 0 ? sysDisplays.FindPrimary().mIndex : monitorIndex;
        const auto& monitor = sysDisplays.Find(monitorIndex);

        resX = resX <= 0 ?  monitor.Width() : resX;
        resY = resY <= 0 ? monitor.Height() : resY;

        yaget::app::WindowAppearance appearance{ yaget::app::Appearance::Fullscreen, resX, resY, monitorIndex };

        appearance.mLeft = monitor.GetRect()->left;
        appearance.mTop = monitor.GetRect()->top;
        appearance.mRight = appearance.mLeft + appearance.mResX;
        appearance.mBottom = appearance.mTop + appearance.mResY;

        return appearance;
    }

    //--------------------------------------------------------------------------------------------------
    //--------------------------------------------------------------------------------------------------
    yaget::app::Appearance LoadWindowAppearance(HWND hWnd, yaget::app::ProcHandler::WindowAppearances& windowAppearances, yaget::io::VirtualTransportSystem& vts, const yaget::io::VirtualTransportSystem::Section& section, const yaget::dev::Configuration::Init& init)
    {
        using namespace yaget;
        
        const app::SysDisplays sysDisplays;
        if (!vts.GetNumTags(section))
        {
            app::Appearance returnValue = app::Appearance::Window;

            int monitorIndex = 0;

            if (init.FullScreen)
            {
                returnValue = app::Appearance::Fullscreen;
            }

            const auto fullscreenAppearance = MakeFullscreenDefault(hWnd, init.mCLO.mResolutionX, init.mCLO.mResolutionY, monitorIndex);
            windowAppearances[fullscreenAppearance.mAppearance] = fullscreenAppearance;

            const auto windowAppearance = MakeWindowDefault(hWnd, init.mCLO.mResolutionX, init.mCLO.mResolutionY, monitorIndex);
            windowAppearances[windowAppearance.mAppearance] = windowAppearance;

            const auto borderlessAppearance = MakeBorderlessDefault(hWnd, monitorIndex);
            windowAppearances[borderlessAppearance.mAppearance] = borderlessAppearance;

            return returnValue;
        }
        else
        {
            auto jsonBlob = io::LoadJson(vts, section);
            windowAppearances = json::GetValue(jsonBlob->root, "Appearances", app::ProcHandler::WindowAppearances{});
            auto active = json::GetValue(jsonBlob->root, "Active", app::Appearance::Window);

            for(auto& [key, appearance] : windowAppearances)
            {
                if (key == active)
                {
                    UpdateFromCLO(appearance, init);
                }

                if (key == app::Appearance::Window)
                {
                    const auto& frameSize = CalculateWindowFrame(hWnd, appearance.mResX, appearance.mResY);
                    appearance.mRight = appearance.mLeft + std::get<0>(frameSize);
                    appearance.mBottom = appearance.mTop + std::get<1>(frameSize);

                    // now we need to validate position of window, is it visible
                    if (!sysDisplays.Intersect({ appearance.mLeft, appearance.mTop, appearance.mRight, appearance.mBottom }))
                    {
                        appearance = MakeDefaultConfiguration(hWnd, appearance.mResX, appearance.mResY);
                    }
                }
                else if (key == app::Appearance::Fullscreen)
                {
                    appearance.mRight = appearance.mLeft +appearance.mResX;
                    appearance.mBottom = appearance.mTop + appearance.mResY;
                    
                }
            }

            return active;
        }
    }

    //--------------------------------------------------------------------------------------------------
    void GetWindowPosition(HWND hWnd, int32_t& windowLeft, int32_t& windowTop, int32_t& windowRight, int32_t& windowBottom, int32_t& resX, int32_t& resY)
    {
        WINDOWPLACEMENT wp = {sizeof(WINDOWPLACEMENT)};
        bool result = ::GetWindowPlacement(hWnd, &wp);
        YAGET_UTIL_THROW_ON_RROR(result, "GetWindowPlacement failed.");

        windowLeft = wp.rcNormalPosition.left;
        windowTop = wp.rcNormalPosition.top;
        windowRight = wp.rcNormalPosition.right;
        windowBottom = wp.rcNormalPosition.bottom;

        RECT rect;
        result = ::GetClientRect(hWnd, &rect);
        YAGET_UTIL_THROW_ON_RROR(result, "GetClientRect failed.");

        resX = rect.right;
        resY = rect.bottom;
    }

    //--------------------------------------------------------------------------------------------------
    void GetWindowResolution(HWND hWnd, int32_t& resX, int32_t& resY)
    {
        int32_t windowLeft, windowTop, windowRight, windowBottom;
        GetWindowPosition(hWnd, windowLeft, windowTop, windowRight, windowBottom, resX, resY);
    }

    //--------------------------------------------------------------------------------------------------
    void SaveWindowAppearance(HWND hWnd, const yaget::app::ProcHandler::WindowAppearances& winAppearances, yaget::app::Appearance appearance, yaget::io::VirtualTransportSystem& vts, const yaget::io::VirtualTransportSystem::Section& section)
    {
        using namespace yaget;
        auto windowAppearances = winAppearances;
        
        nlohmann::json jsonBlock;

        if (appearance == app::Appearance::Window)
        {
            WINDOWPLACEMENT wp = {sizeof(WINDOWPLACEMENT)};
            bool result = ::GetWindowPlacement(hWnd, &wp);
            YAGET_UTIL_THROW_ON_RROR(result, "GetWindowPlacement failed.");
            if (wp.showCmd == SW_MAXIMIZE)
            {
                windowAppearances[appearance].mMaximized = true;
            }
        }

        jsonBlock["Appearances"] = windowAppearances;
        jsonBlock["Active"] = appearance;

        const auto tag = vts.AssureTag(section);
        auto text = json::PrettyPrint(jsonBlock);
        io::Buffer buffer = io::CreateBuffer(text);
        const auto newAsset = io::ResolveAsset<io::JsonAsset>(buffer, tag, vts);
        vts.UpdateAssetData(newAsset, io::VirtualTransportSystem::Request::Add);
    }

    const char* AppearanceNames[] = { "Fullscreen", "Window", "Borderless" };

}

yaget::app::ProcHandler::ProcHandler(const yaget::dev::Configuration::Init& init, const std::string& windowTitle, io::VirtualTransportSystem& vts, ProcessInput processInput, ProcessResize processResize, RequestClose requestClose)
    : mInit(init)
    , mProcessMessage([this](auto&&... params) { return onInitialize(params...); })
    , mProcessInput(std::move(processInput))
    , mProcessResize(std::move(processResize))
    , mRequestClose(std::move(requestClose))
    , mVTS(vts)
{
    using Section = io::VirtualTransportSystem::Section;

    const char* WinName = "WindowClassYaget";

    WNDCLASSEX wc{ sizeof(WNDCLASSEX) };
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = ProcHandler::WindowCallback;
    wc.hInstance = GetModuleHandle(NULL);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    // TODO this may not be needed, since we always paint window (render)
    //wc.hbrBackground = (HBRUSH)COLOR_WINDOW;
    wc.lpszClassName = WinName;

    const auto result = RegisterClassEx(&wc);
    YAGET_UTIL_THROW_ASSERT("WIN", result, "Did not RegisterClassEx");

    HWND winH = ::CreateWindowEx(NULL,
        WinName,                    // name of the window class
        windowTitle.c_str(),        // title of the window
        WS_OVERLAPPEDWINDOW,        // window style
        300,                        // x-position of the window
        300,                        // y-position of the window
        400,                        //static_cast<int>(winSize.x), // width of the window
        400,                        //static_cast<int>(winSize.y), // height of the window
        nullptr,                    // we have no parent window, NULL
        nullptr,                    // we aren't using menus, NULL
        GetModuleHandle(nullptr),   // application handle
        this);               // user data

    YAGET_ASSERT(winH == mWindowHandle, "Return value from CreateWindowEx(...) does not match mWindowHandle.");

    const Section windowSection(init.mWindowOptions);

    mActiveAppearance = LoadWindowAppearance(mWindowHandle, mWindowAppearances, mVTS, windowSection, init);
    const auto& windowAppearance = mWindowAppearances[mActiveAppearance];

    nlohmann::json jsonBlock;
    jsonBlock = mWindowAppearances;
    auto text = json::PrettyPrint(jsonBlock);

    switch (windowAppearance.mAppearance)
    {
    case app::Appearance::Window:
        ::MoveWindow(mWindowHandle, windowAppearance.mLeft, windowAppearance.mTop, windowAppearance.FrameWidth(), windowAppearance.FrameHeight(), FALSE);
        if (windowAppearance.mMaximized)
        {
            mWindowAppearances[mActiveAppearance].mMaximized = false;
            ::ShowWindow(mWindowHandle, SW_MAXIMIZE);
        }
        else
        {
            ::ShowWindow(mWindowHandle, SW_SHOW);
        }
        break;

    case app::Appearance::Borderless:
        {
            mActiveAppearance = app::Appearance::Window;
            const auto& wa = mWindowAppearances[mActiveAppearance];

            ::MoveWindow(mWindowHandle, wa.mLeft, wa.mTop, wa.FrameWidth(), wa.FrameHeight(), FALSE);

            ToggleBorderless();
        }
        break;

    case app::Appearance::Fullscreen:
        // article about full screen in DX11
        // https://docs.microsoft.com/en-us/windows/win32/direct3darticles/dxgi-best-practices
        //::MoveWindow(mWindowHandle, windowAppearance.mLeft, windowAppearance.mTop, windowAppearance.FrameWidth(), windowAppearance.FrameHeight(), FALSE);
        //::ShowWindow(mWindowHandle, SW_SHOW);
        //break;

    default:
        YAGET_UTIL_THROW("WIN", fmt::format("Window Apperance Type: '%d' is not supported.", windowAppearance.mAppearance));
    }

    YLOG_NOTICE("WIN", "Requested surface: '%s', Resolution: (%dx%d)", AppearanceNames[static_cast<int>(mActiveAppearance)], windowAppearance.mResX, windowAppearance.mResY);

    mProcessMessage = [this](auto&&... params){ return onMessage(params...); };
}

void yaget::app::ProcHandler::onResize(bool sizeChanged)
{
    auto& windowAppearance = mWindowAppearances[mActiveAppearance];
    GetWindowPosition(mWindowHandle, windowAppearance.mLeft, windowAppearance.mTop, windowAppearance.mRight, windowAppearance.mBottom, windowAppearance.mResX, windowAppearance.mResY);

    if (sizeChanged)
    {
        mProcessResize();
    }
}

void yaget::app::ProcHandler::onSuspend(bool /*suspended*/)
{
    
}

int64_t yaget::app::ProcHandler::onInitialize(uint32_t message, uint64_t wParam, int64_t lParam)
{
    if (message == WM_CREATE)
    {
        mProcessMessage = [windowHandle = mWindowHandle](auto&&... params){ return DefWindowProc(windowHandle, params...); };
        // in case of error return -1
    }

    return DefWindowProc(mWindowHandle, message, wParam, lParam);
}

int64_t yaget::app::ProcHandler::onMessage(uint32_t message, uint64_t wParam, int64_t lParam)
{
    YAGET_ASSERT(mWindowHandle, "onMessage called with no valid window handle set.");

    switch (message)
    {
        case WM_CLOSE:
            // if we can close the app return 0, otherwise -1 to cancel close request and keep app open
            return mRequestClose() ? 0 : -1;

        case WM_LBUTTONDOWN:
        case WM_LBUTTONUP:
        case WM_MBUTTONDOWN:
        case WM_MBUTTONUP:
        case WM_RBUTTONDOWN:
        case WM_RBUTTONUP:
        case WM_MOUSEMOVE:
        case WM_MOUSEWHEEL:

        case WM_INPUT:
        case WM_KEYDOWN:
        case WM_SYSKEYDOWN:
        case WM_KEYUP:
        case WM_CHAR:
            return mProcessInput(message, wParam, lParam);

        case WM_SIZE:
        {
            int32_t resX, resY;
            GetWindowResolution(mWindowHandle, resX, resY);
            YLOG_DEBUG("WIN", "Proessing SIZE event. Cuurent client resolution: (%dx%d)", resX, resY);

            if (wParam == SIZE_MINIMIZED)
            {
                YLOG_DEBUG("WIN", "    Suspended...");
                mReflectedState.mSuspended = true;
                onSuspend(true);
            }
            else if (mReflectedState.mSuspended)
            {
                YLOG_DEBUG("WIN", "    Restored...");
                mReflectedState.mSuspended = false;
                onSuspend(false);
            }
            else if (!mReflectedState.mMoving || mReflectedState.mAllowUpdateWhileMoving)
            {
                YLOG_DEBUG("WIN", "    Calling application resize callback...");
                onResize(true);
            }

            break;
        }
        case WM_ENTERSIZEMOVE:
            // We want to avoid trying to resizing the swapchain as the user does the 'rubber band' resize
            YLOG_DEBUG("WIN", "WM_ENTERSIZEMOVE called...");
            mReflectedState.mMoving = true;
            GetWindowResolution(mWindowHandle, mReflectedState.mLastResolution.first, mReflectedState.mLastResolution.second);
            break;

        case WM_EXITSIZEMOVE:
            // Here is the other place where you handle the swapchain resize after the user stops using the 'rubber-band' 
            YLOG_DEBUG("WIN", "WM_EXITSIZEMOVE called...");
            mReflectedState.mMoving = false;

            int32_t resX, resY;
            GetWindowResolution(mWindowHandle, resX, resY);
            onResize(mReflectedState.mLastResolution.first != resX || mReflectedState.mLastResolution.second != resY);
            break;
    }

    return DefWindowProc(mWindowHandle, message, wParam, lParam);
}

void yaget::app::ProcHandler::ToggleBorderless()
{
    // TODO We want to use SetWindowPlacement as a generic function, refactor this code out
    // so we also can use in other places when we need to adjust position
    if (mWindowAppearances[mActiveAppearance].mAppearance == Appearance::Window)
    {
        // first we want to get current window appearance, so we can preserve it
        auto& windowAppearance = mWindowAppearances[Appearance::Window];
        GetWindowPosition(mWindowHandle, windowAppearance.mLeft, windowAppearance.mTop, windowAppearance.mRight, windowAppearance.mBottom, windowAppearance.mResX, windowAppearance.mResY);
        
        const app::SysDisplays sysDisplays;
        const auto primary = sysDisplays.FindNearest(mWindowHandle);
        windowAppearance.mMonitorIndex = primary.mIndex;

        LONG lStyle = ::GetWindowLong(mWindowHandle, GWL_STYLE);
        windowAppearance.mWindowStyle = lStyle;

        lStyle &= ~(WS_CAPTION | WS_THICKFRAME | WS_MINIMIZE | WS_MAXIMIZE | WS_SYSMENU);
        ::SetWindowLong(mWindowHandle, GWL_STYLE, lStyle);

        LONG lExStyle = ::GetWindowLong(mWindowHandle, GWL_EXSTYLE);
        windowAppearance.mWindowExStyle = lExStyle;

        lExStyle &= ~(WS_EX_DLGMODALFRAME | WS_EX_CLIENTEDGE | WS_EX_STATICEDGE);
        ::SetWindowLong(mWindowHandle, GWL_EXSTYLE, lExStyle);

        ::PostMessage(mWindowHandle, WM_SYSCOMMAND, SC_MAXIMIZE, 0);

        mActiveAppearance = Appearance::Borderless;
        WindowAppearance borderlessAppearance;
        borderlessAppearance.mAppearance = mActiveAppearance;
        borderlessAppearance.mLeft = 0;
        borderlessAppearance.mTop = 0;
        borderlessAppearance.mRight = primary.Width();
        borderlessAppearance.mBottom = primary.Height();
        borderlessAppearance.mResX = primary.Width();
        borderlessAppearance.mResY = primary.Height();
        borderlessAppearance.mMonitorIndex = primary.mIndex;

        mWindowAppearances[mActiveAppearance] = borderlessAppearance;
    }
    else if (mWindowAppearances[mActiveAppearance].mAppearance == Appearance::Borderless)
    {
        const auto& windowAppearance = mWindowAppearances[Appearance::Window];

        ::SetWindowLong(mWindowHandle, GWL_STYLE, windowAppearance.mWindowStyle);
        ::SetWindowLong(mWindowHandle, GWL_EXSTYLE, windowAppearance.mWindowExStyle);
        
        WINDOWPLACEMENT wp = { sizeof(WINDOWPLACEMENT) };
        bool result = ::GetWindowPlacement(mWindowHandle, &wp);
        YAGET_UTIL_THROW_ON_RROR(result, "GetWindowPlacement failed in respose to ToggleBorderless.");

        wp.rcNormalPosition.left = windowAppearance.mLeft;
        wp.rcNormalPosition.top = windowAppearance.mTop;
        wp.rcNormalPosition.right = windowAppearance.mRight;
        wp.rcNormalPosition.bottom = windowAppearance.mBottom;
        result = ::SetWindowPlacement(mWindowHandle, &wp);
        YAGET_UTIL_THROW_ON_RROR(result, "SetWindowPlacement failed in respose to ToggleBorderless.");

        mActiveAppearance = Appearance::Window;
    }
}


void yaget::app::ProcHandler::SaveAppearance() const
{
    SaveWindowAppearance(mWindowHandle, mWindowAppearances, mActiveAppearance, mVTS, mInit.mWindowOptions);
}


/*static*/ LRESULT CALLBACK yaget::app::ProcHandler::WindowCallback(HWND hWnd, uint32_t message, uint64_t wParam, int64_t lParam)
{
        if (auto This = GetThis(hWnd))
        {
            YAGET_ASSERT(This->mProcessMessage, "There is no callback associated with window proc.");

            if (message == WM_DESTROY)
            {
                This->SaveAppearance();

                // close the application entirely
                SetWindowLongPtr(hWnd, GWLP_USERDATA, 0);
                PostQuitMessage(0);

                // if we processed this message return 0
                return 0;
            }
            else if (message == WM_GETMINMAXINFO)
            {
                // We want to prevent the window from being set too tiny
                auto info = reinterpret_cast<MINMAXINFO*>(lParam);
                info->ptMinTrackSize.x = SafeResolutionX;
                info->ptMinTrackSize.y = SafeResolutionY;

                // if we processed this message return 0
                return 0;
            }
            else if (message == WM_DPICHANGED)
            {
                const auto currentDpi = static_cast<float>(HIWORD(wParam));
                UpdateDpiConfiguration(currentDpi);
                // 
                This->onResize(true);
            }
            else if ((message == WM_SYSKEYDOWN || message == WM_KEYDOWN) && wParam == VK_RETURN && (HIWORD(lParam) & KF_ALTDOWN))
            {
                This->ToggleBorderless();

                return 0;
            }
            else
            {
                return This->mProcessMessage(message, wParam, lParam);
            }
        }
        else
        {
            if (message == WM_NCCREATE)
            {
                SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR)((CREATESTRUCT*)lParam)->lpCreateParams);
                auto windowHandler = reinterpret_cast<app::ProcHandler *>((LONG_PTR)((CREATESTRUCT*)lParam)->lpCreateParams);
                windowHandler->mWindowHandle = hWnd;

                if (HMONITOR monitor = ::MonitorFromWindow(hWnd, MONITOR_DEFAULTTONEAREST))
                {
                    uint32_t dpiX, dpiY;
                    HRESULT hr = ::GetDpiForMonitor(monitor, MDT_EFFECTIVE_DPI, &dpiX, &dpiY);
                    YAGET_UTIL_THROW_ON_RROR(hr, "Did not GetDpiForMonitor.");
                    UpdateDpiConfiguration(static_cast<float>(dpiX));
                }

                // in case of an error, return FALSE to stop creation of this window
            }
        }

        // Handle any messages the switch statement didn't
        return DefWindowProc(hWnd, message, wParam, lParam);
}
