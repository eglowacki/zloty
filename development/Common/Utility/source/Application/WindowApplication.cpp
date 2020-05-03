#include "App/WindowApplication.h"
#include "App/AppUtilities.h"
#include "Platform/Support.h"
#include "Platform/WindowsLean.h"
#include "VTS/VirtualTransportSystem.h"
#include "VTS/ResolvedAssets.h"
#include "Debugging/DevConfiguration.h"
#include "Json/JsonHelpers.h"
#include "MathFacade.h"

#include <Windowsx.h>
#include <shellscalingapi.h>

using namespace yaget;
using namespace DirectX;

namespace
{
    //--------------------------------------------------------------------------------------------------
    struct WindowFlags
    {
        bool Created = false;
        bool Moving = false;
        bool Minimized = false;
        bool Maximized = false;
        bool Fullscreen = false;
        bool CtorIsRunning = false;
    };
    WindowFlags sWindowFlags;

    //--------------------------------------------------------------------------------------------------
    WindowApplication* GetWindowApplication(HWND hWnd)
    {
        return sWindowFlags.Created ? reinterpret_cast<WindowApplication *>(::GetWindowLongPtr(hWnd, GWLP_USERDATA)) : nullptr;
    }

    //--------------------------------------------------------------------------------------------------
    class MonitorInfoEx : public MONITORINFOEX
    {
    public:
        MonitorInfoEx()
        {
            cbSize = sizeof(MONITORINFOEX);
        }

        const RECT* GetRect() const { return &rcMonitor; }
        const RECT* GetWorkRect() const { return &rcWork; }
        LPCTSTR DeviceName() const { return szDevice; }

        bool IsPrimary() const { return (dwFlags & MONITORINFOF_PRIMARY) ? true : false; }

        int Width() const { return rcMonitor.right - rcMonitor.left; }
        int Height() const { return rcMonitor.bottom - rcMonitor.top; }
        int WorkWidth() const { return rcWork.right - rcWork.left; }
        int WorkHeight() const { return rcWork.bottom - rcWork.top; }

        HMONITOR hMonitor = nullptr;
    };
    
    //--------------------------------------------------------------------------------------------------
    BOOL CALLBACK MonitorEnumProc(__in  HMONITOR hMonitor, __in  HDC hdcMonitor, __in  LPRECT lprcMonitor, __in  LPARAM dwData)
    {
        hdcMonitor;
        lprcMonitor;

        std::vector<MonitorInfoEx>& infoArray = *reinterpret_cast<std::vector<MonitorInfoEx>*>(dwData);
        MonitorInfoEx info;
        ::GetMonitorInfo(hMonitor, &info);
        info.hMonitor = hMonitor;
        infoArray.push_back(info);
        return TRUE;
    }

    //--------------------------------------------------------------------------------------------------
    class SysDisplays
    {
    public:
        SysDisplays()
        {
            Update();
        }

        bool Intersect(const RECT& windowPos) const
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

        void Update()
        {
            mInfo.clear();
            mInfo.reserve(::GetSystemMetrics(SM_CMONITORS));
            ::EnumDisplayMonitors(nullptr, nullptr, MonitorEnumProc, reinterpret_cast<LPARAM>(&mInfo));
        }

        int Count() const
        {
            return static_cast<int>(mInfo.size());
        }

        const MonitorInfoEx& Get(int i) const
        {
            return mInfo[i];
        }

    private:
        std::vector<MonitorInfoEx> mInfo;
    };

    //--------------------------------------------------------------------------------------------------
    SimpleMath::Vector2 CalculateWindowFrame(HWND hWnd, int w, int h)
    {
        RECT rcClient, rcWind;
        POINT ptDiff;
        ::GetClientRect(hWnd, &rcClient);
        ::GetWindowRect(hWnd, &rcWind);
        ptDiff.x = (rcWind.right - rcWind.left) - rcClient.right;
        ptDiff.y = (rcWind.bottom - rcWind.top) - rcClient.bottom;
        return math3d::Vector2(static_cast<float>(w + ptDiff.x), static_cast<float>(h + ptDiff.y));
    }

    //--------------------------------------------------------------------------------------------------
    void SaveWindowPostion(HWND hWnd, io::VirtualTransportSystem& vts)
    {
        using namespace yaget;
        using Section = io::VirtualTransportSystem::Section;

        WINDOWPLACEMENT wp = {};
        if (::GetWindowPlacement(hWnd, &wp))
        {
            YLOG_INFO("WIN", "Saved Window Position: Top = %d, Left = %d, Bottom = %d, Right = %d, State: = %d.", 
                wp.rcNormalPosition.top, wp.rcNormalPosition.left, 
                wp.rcNormalPosition.bottom, wp.rcNormalPosition.right, wp.showCmd);

            nlohmann::json jsonBlock;

            jsonBlock["Maximized"] = sWindowFlags.Maximized;
            jsonBlock["Minimized"] = sWindowFlags.Minimized;
            jsonBlock["Fullscreen"] = sWindowFlags.Fullscreen;
            jsonBlock["Top"] = wp.rcNormalPosition.top;
            jsonBlock["Left"] = wp.rcNormalPosition.left;
            jsonBlock["Bottom"] = wp.rcNormalPosition.bottom;
            jsonBlock["Right"] = wp.rcNormalPosition.right;

            RECT rect;
            if (GetClientRect(hWnd, &rect))
            {
                jsonBlock["ResolutionX"] = rect.right;
                jsonBlock["ResolutionY"] = rect.bottom;
            }

            const Section windowSection(dev::CurrentConfiguration().mInit.mWindowOptions);
            io::Tag tag = vts.GetTag(windowSection);
            if (!tag.IsValid())
            {
                tag = vts.GenerateTag(windowSection);
            }

            auto text = json::PrettyPrint(jsonBlock);
            io::Buffer buffer = io::CreateBuffer(text);
            auto newAsset = io::ResolveAsset<io::JsonAsset>(buffer, tag, vts);
            vts.UpdateAssetData(newAsset, io::VirtualTransportSystem::Request::Add);
        }
    }

    //--------------------------------------------------------------------------------------------------
    WindowApplication::Surface LoadWindowPosition(HWND hWnd, int requestedWidth, int requestedHeight, io::VirtualTransportSystem& vts)
    {
        using namespace yaget;
        using Section = io::VirtualTransportSystem::Section;

        bool maximized = false;
        bool fullScreen = false;
        bool minimized = false;
        int width = requestedWidth, height = requestedHeight;
        int left = 300, top = 300;
        int bottom = left + width, right = top + height;

        const Section windowSection(dev::CurrentConfiguration().mInit.mWindowOptions);

        io::SingleBLobLoader<io::JsonAsset> configurationLoader(vts, windowSection);
        auto asset = configurationLoader.GetAsset();
        if (asset)
        {
            const auto& jsonBlock = asset->root;

            maximized = json::GetValue(jsonBlock, "Maximized", maximized);
            minimized = json::GetValue(jsonBlock, "Minimized", minimized);
            fullScreen = json::GetValue(jsonBlock, "Fullscreen", fullScreen);
            top = json::GetValue(jsonBlock, "Top", top);
            left = json::GetValue(jsonBlock, "Left", left);
            bottom = json::GetValue(jsonBlock, "Bottom", bottom);
            right = json::GetValue(jsonBlock, "Right", right);
        }

        if (width == 0)
        {
            // load right and bottom and calculate width and height
        }

        // the following correction is needed when the taskbar is
        // at the left or top and it is not "auto-hidden"
        RECT workArea;
        SystemParametersInfo(SPI_GETWORKAREA, 0, &workArea, 0);
        left += workArea.left;
        top += workArea.top;

        YLOG_INFO("WIN", "Loaded Window Position: Top = %d, Left = %d, Bottom = %d, Right = %d", top, left, top + height, left + width);

        ::MoveWindow(hWnd, left, top, width, height, FALSE);

        // let's make sure that window is not out of view, and if it is, center it on primary monitor
        SysDisplays attachedDisplayes;
        if (!attachedDisplayes.Intersect({ left, top, right, bottom }))
        {
            asset = nullptr;
        }

        // if there was no window configuration file, we want to center 
        // our window on primary monitor
        if (!asset)
        {
            HMONITOR windowMonitor = ::MonitorFromWindow(hWnd, MONITOR_DEFAULTTONEAREST);
            MONITORINFO monitorInfo = { sizeof(MONITORINFO) };
            if (GetMonitorInfo(windowMonitor, &monitorInfo))
            {
                uint32_t monitorWidth = monitorInfo.rcWork.right - monitorInfo.rcWork.left;
                uint32_t monitorHeight = monitorInfo.rcWork.bottom - monitorInfo.rcWork.top;

                ::MoveWindow(hWnd, (monitorWidth/2) - (width/2), (monitorHeight/2) - (height/2), width, height, FALSE);
            }
        }

        WindowApplication::Surface windowState = WindowApplication::Surface::Window;
        if (fullScreen && maximized)
        {
            windowState = WindowApplication::Surface::Borderless;
        }
        else if (!fullScreen && maximized)
        {
            windowState = WindowApplication::Surface::Maximized;
        }
        else if (fullScreen && !maximized)
        {
            windowState = WindowApplication::Surface::Fullscreen;
        }
        else if (minimized)
        {
            windowState = WindowApplication::Surface::Minimized;
        }

        return windowState;
    }
    
    //--------------------------------------------------------------------------------------------------
    void UpdateDpiConfiguration(float currentDpi)
    {
        dev::CurrentConfiguration().mRuntime.RefreshDpi(currentDpi / USER_DEFAULT_SCREEN_DPI);
        YLOG_INFO("WIN", "DPI Scale set at: '%.2f'.", yaget::dev::CurrentConfiguration().mRuntime.DpiScaleFactor);
    }

    
    //--------------------------------------------------------------------------------------------------
    // this is the main message handler for the program
    LRESULT CALLBACK YagetWindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
    {
        // sort through and find what code to run for the message given
        switch (message)
        {
            case WM_NCCREATE:
            {
                SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR)((CREATESTRUCT*)lParam)->lpCreateParams);
                sWindowFlags.Created = true;
                if (HMONITOR monitor = ::MonitorFromWindow(hWnd, MONITOR_DEFAULTTONEAREST))
                {
                    uint32_t dpiX, dpiY;
                    HRESULT hr = ::GetDpiForMonitor(monitor, MDT_EFFECTIVE_DPI, &dpiX, &dpiY);
                    YAGET_UTIL_THROW_ON_RROR(hr, "Did not GetDpiForMonitor.");
                    UpdateDpiConfiguration(static_cast<float>(dpiX));
                }
                break;
            }
            case WM_CLOSE:
            {
                // app is requesting to be closed
                if (WindowApplication* windowApplication = GetWindowApplication(hWnd))
                {
                    windowApplication->RequestQuit();
                }
                return 0;
            }
            case WM_DESTROY: // this message is read when the window is closed
            {
                // close the application entirely
                if (WindowApplication* windowApplication = GetWindowApplication(hWnd))
                {
                    SaveWindowPostion(hWnd, windowApplication->VTS());
                }

                PostQuitMessage(0);

                return 0;
            }

            case WM_LBUTTONDOWN:
            case WM_LBUTTONUP:
            case WM_MBUTTONDOWN:
            case WM_MBUTTONUP:
            case WM_RBUTTONDOWN:
            case WM_RBUTTONUP:
            case WM_MOUSEMOVE:
            case WM_MOUSEWHEEL:
            {
                if (WindowApplication* windowApplication = GetWindowApplication(hWnd))
                {
                    int64_t result = windowApplication->_onHandleInputMessage(hWnd, message, wParam, lParam);
                    if (result)
                    {
                        return 0;
                    }
                    else if (windowApplication->_processMouseMessage(message, wParam, lParam) == 0)
                    {
                        return 0;
                    }
                }
                break;
            }
            case WM_INPUT:
            case WM_KEYDOWN:
            case WM_SYSKEYDOWN:
            case WM_KEYUP:
            case WM_CHAR:
                if (WindowApplication* windowApplication = GetWindowApplication(hWnd))
                {
                    if (message == WM_INPUT)
                    {
                        if (windowApplication->_processInputMessage(lParam) == 0)
                        {
                            return 0;
                        }
                    }
                    else if ((message == WM_SYSKEYDOWN || message == WM_KEYDOWN) && wParam == VK_RETURN && (HIWORD(lParam) & KF_ALTDOWN))
                    {
                        YLOG_DEBUG("WIN", "Alt-Enter Pressed!");

                        windowApplication->_onToggleFullScreen();
                        return 0;
                    }
                    else
                    {
                        int64_t result = windowApplication->_onHandleInputMessage(hWnd, message, wParam, lParam);
                        if (result)
                        {
                            return 0;
                        }
                    }
                }
                break;
            case WM_SIZE:
            {
                if (WindowApplication* windowApplication = GetWindowApplication(hWnd))
                {
                    bool bWasMinimized = sWindowFlags.Minimized;

                    if (wParam == SIZE_MAXIMIZED)
                    {
                        sWindowFlags.Maximized = true;
                        sWindowFlags.Minimized = false;

                        windowApplication->_onResise();
                    }
                    else if (wParam == SIZE_MINIMIZED)
                    {
                        sWindowFlags.Minimized = true;
                        sWindowFlags.Maximized = false;

                        windowApplication->_onSuspend(true);
                    }
                    else if (wParam == SIZE_RESTORED && (sWindowFlags.Minimized || sWindowFlags.Maximized))
                    {
                        sWindowFlags.Minimized = false;
                        sWindowFlags.Maximized = false;

                        windowApplication->_onResise();
                    }

                    if (bWasMinimized && !sWindowFlags.Minimized)
                    {
                        windowApplication->_onSuspend(false);
                    }

                }
                break;
            }
            case WM_ENTERSIZEMOVE:
                // We want to avoid trying to resizing the swapchain as the user does the 'rubber band' resize
                YLOG_DEBUG("WIN", "WM_ENTERSIZEMOVE called...");
                sWindowFlags.Moving = true;
                break;

            case WM_EXITSIZEMOVE:
                // Here is the other place where you handle the swapchain resize after the user stops using the 'rubber-band' 
                YLOG_DEBUG("WIN", "WM_EXITSIZEMOVE called...");
                sWindowFlags.Moving = false;

                if (WindowApplication* windowApplication = GetWindowApplication(hWnd))
                {
                    windowApplication->_onResise();
                }
                break;
            case WM_GETMINMAXINFO:
            {
                // We want to prevent the window from being set too tiny
                auto info = reinterpret_cast<MINMAXINFO*>(lParam);
                info->ptMinTrackSize.x = 320;
                info->ptMinTrackSize.y = 200;
                break;
            }
            case WM_DPICHANGED:
            {
                float currentDpi = static_cast<float>(HIWORD(wParam));
                UpdateDpiConfiguration(currentDpi);
                break;
            }        
        }

        // Handle any messages the switch statement didn't
        return DefWindowProc(hWnd, message, wParam, lParam);
    }

    void UpdateInputFlags(uint32_t& lastFlags, uint32_t newFlags, uint32_t flag)
    {
        if (newFlags & input::kButtonDown)
        {
            if (newFlags & flag)
            {
                lastFlags |= flag;
            }
        }
        else if (newFlags & input::kButtonUp)
        {
            if (newFlags & flag)
            {
                lastFlags &= ~flag;
            }
        }
    }


    void GetWindowPostion(HWND hWnd, int32_t& windowLeft, int32_t& windowTop, int32_t& windowRight, int32_t& windowBottom)
    {
        WINDOWPLACEMENT wp = {sizeof(WINDOWPLACEMENT)};
        bool result = ::GetWindowPlacement(hWnd, &wp);
        YAGET_UTIL_THROW_ON_RROR(result, "GetWindowPlacement failed.");

        windowLeft = wp.rcNormalPosition.left;
        windowTop = wp.rcNormalPosition.top;
        windowRight = wp.rcNormalPosition.right;
        windowBottom = wp.rcNormalPosition.bottom;
    }


} // namespace


math3d::Vector2 WindowApplication::GetWindowSize() const
{
    if (!mWinHandle)
    {
        return math3d::Vector2(1, 1);
    }

    RECT clientRect;
    ::GetClientRect(GetWindowHandle<HWND>(), &clientRect);

    // Compute the exact client dimensions.
    // This is required for a correct projection matrix.
    SimpleMath::Vector2 size;
    int w = std::max<int>(1, clientRect.right - clientRect.left);
    int h = std::max<int>(1, clientRect.bottom - clientRect.top);
    size.x = static_cast<float>(w);
    size.y = static_cast<float>(h);

    return size;
}

void WindowApplication::_onToggleFullScreen()
{
    if (mWinHandle)
    {
        HWND hWin = GetWindowHandle<HWND>();
        if (mFullScreen)
        {
            ::SetWindowLong(hWin, GWL_STYLE, mWindowStyle);
            ::SetWindowLong(hWin, GWL_EXSTYLE, mWindowExStyle);

            WINDOWPLACEMENT wp = { sizeof(WINDOWPLACEMENT) };
            bool result = ::GetWindowPlacement(hWin, &wp);
            YAGET_UTIL_THROW_ON_RROR(result, "GetWindowPlacement failed in respose to _onToggleFullScreen.");

            wp.rcNormalPosition.left = mWindowLeft;
            wp.rcNormalPosition.top = mWindowTop;
            wp.rcNormalPosition.right = mWindowRight;
            wp.rcNormalPosition.bottom = mWindowBottom;
            result = ::SetWindowPlacement(hWin, &wp);
            YAGET_UTIL_THROW_ON_RROR(result, "SetWindowPlacement failed in respose to _onToggleFullScreen.");
        }
        else
        {
            GetWindowPostion(hWin, mWindowLeft, mWindowTop, mWindowRight, mWindowBottom);

            LONG lStyle = ::GetWindowLong(hWin, GWL_STYLE);
            mWindowStyle = lStyle;
            lStyle &= ~(WS_CAPTION | WS_THICKFRAME | WS_MINIMIZE | WS_MAXIMIZE | WS_SYSMENU);
            ::SetWindowLong(hWin, GWL_STYLE, lStyle);
            LONG lExStyle = ::GetWindowLong(hWin, GWL_EXSTYLE);
            mWindowExStyle = lExStyle;
            lExStyle &= ~(WS_EX_DLGMODALFRAME | WS_EX_CLIENTEDGE | WS_EX_STATICEDGE);
            ::SetWindowLong(hWin, GWL_EXSTYLE, lExStyle);

            ::PostMessage(hWin, WM_SYSCOMMAND, SC_MAXIMIZE, 0);
        }
        mFullScreen = !mFullScreen;
        sWindowFlags.Fullscreen = mFullScreen;
    }
}

int64_t WindowApplication::_onHandleInputMessage(WindowHandle_t hWnd, uint32_t message, uint64_t wParam, int64_t lParam)
{
    if (mWinHandle && !sWindowFlags.CtorIsRunning)
    {
        return onHandleRawInput(hWnd, message, wParam, lParam);
    }

    return 0;
}

void WindowApplication::_onSuspend(bool /*bSuspend*/)
{
    if (mWinHandle)
    { }
}

void WindowApplication::_onResise()
{
    // crappy way of handling to not call into Device while 
    // this object is getting constructed (CtorIsRunning)
    if (mWinHandle && !sWindowFlags.CtorIsRunning)  
    {
        OnResize();
    }
}

WindowApplication::WindowApplication(const std::string& title, items::Director& director, io::VirtualTransportSystem& vts, const args::Options& options)
    : Application(title, director, vts, options)
{
    sWindowFlags.CtorIsRunning = true;
#if 0//(_WIN32_WINNT >= 0x0A00 /*_WIN32_WINNT_WIN10*/)
    Microsoft::WRL::Wrappers::RoInitializeWrapper initialize(RO_INIT_MULTITHREADED);
    //YAGET_UTIL_THROW_ON_RROR(hr, "Did not CoInitialize COM");
#else
    HRESULT hr = ::CoInitializeEx(nullptr, COINITBASE_MULTITHREADED);
    YAGET_UTIL_THROW_ON_RROR(hr, "Did not CoInitialize COM");
#endif

    SysDisplays attachedDisplayes;

    // the handle for the window, filled by a function
    //HWND hWnd;
    // this struct holds information for the window class
    WNDCLASSEX wc = {};

    // fill in the struct with the needed information
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = YagetWindowProc;
    wc.hInstance = GetModuleHandle(NULL);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)COLOR_WINDOW;
    wc.lpszClassName = "WindowClassYaget";

    // register the window class
    ::RegisterClassEx(&wc);

    // create the window and use the result as the handle
    HWND winH = ::CreateWindowEx(NULL,
        "WindowClassYaget",         // name of the window class
        title.c_str(),              // title of the window
        WS_OVERLAPPEDWINDOW,        // window style
        300,                        // x-position of the window
        300,                        // y-position of the window
        400,                        //static_cast<int>(winSize.x), // width of the window
        400,                        //static_cast<int>(winSize.y), // height of the window
        NULL,                       // we have no parent window, NULL
        NULL,                       // we aren't using menus, NULL
        GetModuleHandle(NULL),      // application handle
        this);                      // used with multiple windows, NULL

    int resX = dev::CurrentConfiguration().mInit.ResX;
    int resY = dev::CurrentConfiguration().mInit.ResY;

    SimpleMath::Vector2 winSize = CalculateWindowFrame(winH, resX, resY);
    Surface requestedWindowState = LoadWindowPosition(winH, static_cast<int>(winSize.x), static_cast<int>(winSize.y), VTS());

    RAWINPUTDEVICE Rid[1];

    Rid[0].usUsagePage = 0x01;
    Rid[0].usUsage = 0x06;
    Rid[0].dwFlags = 0;// RIDEV_NOLEGACY;    // adds HID keyboard and also ignores legacy mouse messages
    Rid[0].hwndTarget = winH;

    //Rid[1].usUsagePage = 0x01;
    //Rid[1].usUsage = 0x02;
    //Rid[1].dwFlags = RIDEV_INPUTSINK;// RIDEV_INPUTSINK;// RIDEV_NOLEGACY;   // adds HID mouse and also ignores legacy mouse messages
    //Rid[1].hwndTarget = winH;

    ::RegisterRawInputDevices(Rid, 1, sizeof(Rid[0]));
    // display the window on the screen

    mWinHandle = winH;
    switch (requestedWindowState)
    {
    case Surface::Window:

        YLOG_INFO("WIN", "Activated Window State");
        ShowWindow(winH, SW_SHOW);
        break;
    case Surface::Borderless:
        YLOG_INFO("WIN", "Toggling fullscreen to activate: '%s'.", mFullScreen ? "Fullscreen" : "Windowed");
        _onToggleFullScreen();
        break;
    case Surface::Fullscreen:
        mSurface = Surface::Fullscreen;
        ShowWindow(winH, SW_SHOW);
        util::DisplayDialog("Video Error", "Fullscreen mode is not implemented yet!!!");
        YLOG_ERROR("WIN", "Fullscreen mode is not implemented yet!!!");
        break;
    case Surface::Maximized:
        YLOG_INFO("WIN", "Maximizing Window State");
        ShowWindow(winH, SW_MAXIMIZE);
        break;
    case Surface::Minimized:
        YLOG_INFO("WIN", "Minimizing Window State");
        ShowWindow(winH, SW_MINIMIZE);
        break;
    default:
        break;
    }

    sWindowFlags.CtorIsRunning = false;
}

WindowApplication::~WindowApplication()
{ 
    sWindowFlags.Created = false;
}

int WindowApplication::_processMouseMessage(uint32_t message, uint64_t wParam, int64_t lParam)
{
    uint32_t inputFlags = 0;
    int zDelta = 0;
    input::InputDevice::Mouse::Buttons buttons;

    if (wParam & MK_CONTROL)
    {
        inputFlags |= input::kButtonCtrl;
    }
    if (wParam & MK_SHIFT)
    {
        inputFlags |= input::kButtonShift;
    }
    if (wParam & MK_LBUTTON)
    {
        buttons[input::kMouseLeft] = true;
    }
    if (wParam & MK_MBUTTON)
    {
        buttons[input::kMouseMiddle] = true;
    }
    if (wParam & MK_RBUTTON)
    {
        buttons[input::kMouseRight] = true;
    }
    if (wParam & MK_XBUTTON1)
    {
        buttons[input::kMouse4] = true;
    }
    if (wParam & MK_XBUTTON2)
    {
        buttons[input::kMouse5] = true;
    }

    if (message == WM_LBUTTONDOWN)
    {
        inputFlags |= input::kButtonDown;
        buttons[input::kMouseLeft] = true;
    }
    else if (message == WM_LBUTTONUP)
    {
        inputFlags |= input::kButtonUp;
        buttons[input::kMouseLeft] = true;
    }
    else if (message == WM_MBUTTONDOWN)
    {
        inputFlags |= input::kButtonDown;
        buttons[input::kMouseMiddle] = true;
    }
    else if (message == WM_MBUTTONUP)
    {
        inputFlags |= input::kButtonUp;
        buttons[input::kMouseMiddle] = true;
    }
    else if (message == WM_RBUTTONDOWN)
    {
        inputFlags |= input::kButtonDown;
        buttons[input::kMouseRight] = true;
    }
    else if (message == WM_RBUTTONUP)
    {
        inputFlags |= input::kButtonUp;
        buttons[input::kMouseRight] = true;
    }
    else if (message == WM_XBUTTONDOWN)
    {
        inputFlags |= input::kButtonDown;
        buttons[input::kMouse4] = true;
    }
    else if (message == WM_XBUTTONUP)
    {
        inputFlags |= input::kButtonUp;
        buttons[input::kMouse4] = true;
    }
    else if (message == WM_MOUSEMOVE)
    {
        inputFlags |= input::kMouseMove;
    }
    else if (message == WM_MOUSEWHEEL)
    {
        inputFlags |= input::kMouseWheel;
        zDelta = GET_WHEEL_DELTA_WPARAM(wParam);
    }

    int xPos = GET_X_LPARAM(lParam);
    int yPos = GET_Y_LPARAM(lParam);

    Mouse::Location mouseLocation(xPos, yPos);
    std::unique_ptr<Mouse> mouseInput = std::make_unique<Mouse>(inputFlags, buttons, zDelta, mouseLocation);
    if (!mLastMouseInput)
    {
        mLastMouseInput = std::make_unique<Mouse>(static_cast<uint32_t>(-1), buttons, zDelta, mouseLocation);
    }

    if (inputFlags & input::kMouseWheel ||
        mouseInput->mFlags != mLastMouseInput->mFlags || 
        mouseInput->mButtons != mLastMouseInput->mButtons || 
        mouseInput->mZDelta != mLastMouseInput->mZDelta ||
        mouseInput->mPos != mLastMouseInput->mPos)
    {
        Mouse::Location diffLocation(mouseInput->mPos.x - mLastMouseInput->mPos.x, mouseInput->mPos.y - mLastMouseInput->mPos.y);
        mInputDevice.MouseRecord(inputFlags, buttons, zDelta, diffLocation, platform::GetRealTime(time::kMicrosecondUnit));
        std::swap(mLastMouseInput, mouseInput);
    }

    return 0;
}

int WindowApplication::_processInputMessage(int64_t lParam)
{
    char buffer[sizeof(RAWINPUT)] = {};
    UINT size = sizeof(RAWINPUT);
    GetRawInputData(reinterpret_cast<HRAWINPUT>(lParam), RID_INPUT, buffer, &size, sizeof(RAWINPUTHEADER));

    // extract keyboard raw input data
    RAWINPUT* raw = reinterpret_cast<RAWINPUT*>(buffer);
    if (raw->header.dwType == RIM_TYPEKEYBOARD)
    {
        uint32_t inputFlags = 0;

        const RAWKEYBOARD& rawKB = raw->data.keyboard;
        // do something with the data here

        UINT virtualKey = rawKB.VKey;
        UINT scanCode = rawKB.MakeCode;
        UINT flags = rawKB.Flags;

        if (virtualKey == 255)
        {
            // discard "fake keys" which are part of an escaped sequence
            return 0;
        }
        else if (virtualKey == VK_SHIFT)
        {
            // correct left-hand / right-hand SHIFT
            virtualKey = MapVirtualKey(scanCode, MAPVK_VSC_TO_VK_EX);
            inputFlags |= input::kButtonShift;
        }
        else if (virtualKey == VK_NUMLOCK)
        {
            // correct PAUSE/BREAK and NUM LOCK silliness, and set the extended bit
            scanCode = (MapVirtualKey(virtualKey, MAPVK_VK_TO_VSC) | 0x100);
        }

        // e0 and e1 are escape sequences used for certain special keys, such as PRINT and PAUSE/BREAK.
        // see http://www.win.tue.nl/~aeb/linux/kbd/scancodes-1.html
        const bool isE0 = ((flags & RI_KEY_E0) != 0);
        const bool isE1 = ((flags & RI_KEY_E1) != 0);

        if (isE1)
        {
            // for escaped sequences, turn the virtual key into the correct scan code using MapVirtualKey.
            // however, MapVirtualKey is unable to map VK_PAUSE (this is a known bug), hence we map that by hand.
            if (virtualKey == VK_PAUSE)
            {
                scanCode = 0x45;
            }
            else
            {
                scanCode = MapVirtualKey(virtualKey, MAPVK_VK_TO_VSC);
            }
        }

        switch (virtualKey)
        {
            case VK_MENU:
                inputFlags |= input::kButtonAlt;
                break;
            case VK_CONTROL:
                inputFlags |= input::kButtonCtrl;
                break;
            case VK_RETURN:
                virtualKey = input::kReturn;
                break;
        }

        // a key can either produce a "make" or "break" scancode. this is used to differentiate between down-presses and releases
        // see http://www.win.tue.nl/~aeb/linux/kbd/scancodes-1.html
        const bool wasUp = ((flags & RI_KEY_BREAK) != 0);

        int keyValue = mInputDevice.MapKey(virtualKey);

        if (!wasUp)
        {
            if (!mLastKeyState[keyValue])
            {
                mLastKeyState[keyValue] = true;
                inputFlags |= input::kButtonDown;
                mLastKeyFlags |= input::kButtonDown;
                mLastKeyFlags &= ~input::kButtonUp;
            }
            else
            {
                // skip this key
                return 0;
            }
        }
        else
        {
            mLastKeyState[keyValue] = false;
            inputFlags |= input::kButtonUp;
            mLastKeyFlags |= input::kButtonUp;
            mLastKeyFlags &= ~input::kButtonDown;
        }

        // getting a human-readable string
        UINT key = (scanCode << 16) | (isE0 << 24);
        char keyName[512] = {};
        GetKeyNameText(static_cast<LONG>(key), keyName, 512);

        UpdateInputFlags(mLastKeyFlags, inputFlags, input::kButtonShift);
        UpdateInputFlags(mLastKeyFlags, inputFlags, input::kButtonCtrl);
        UpdateInputFlags(mLastKeyFlags, inputFlags, input::kButtonAlt);

        mInputDevice.KeyRecord(mLastKeyFlags, keyValue);

        return 0;
    }
#if 0
    else if (raw->header.dwType == RIM_TYPEMOUSE)
    {
        input::InputDevice::Mouse::Location mousePos(0, 0);
        BitArray buttons(32);
        uint32_t inputFlags = 0;
        const RAWMOUSE& rawMouse = raw->data.mouse;

        if (rawMouse.usFlags & MOUSE_MOVE_ABSOLUTE)
        {
            mousePos.x = rawMouse.lLastX;
            mousePos.y = rawMouse.lLastY;
        }
        else if (rawMouse.usFlags & MOUSE_MOVE_RELATIVE)
        {
            YAGET_ASSERT(false);
        }

        if (rawMouse.usButtonFlags & RI_MOUSE_LEFT_BUTTON_DOWN)
        {
            inputFlags |= input::kButtonDown;
            buttons[input::kMouseLeft] = true;
        }
        else if (rawMouse.usButtonFlags & RI_MOUSE_LEFT_BUTTON_UP)
        {
            inputFlags |= input::kButtonUp;
            buttons[input::kMouseLeft] = true;
        }

        if (rawMouse.usButtonFlags & RI_MOUSE_RIGHT_BUTTON_DOWN)
        {
            inputFlags |= input::kButtonDown;
            buttons[input::kMouseRight] = true;
        }
        else if (rawMouse.usButtonFlags & RI_MOUSE_RIGHT_BUTTON_UP)
        {
            inputFlags |= input::kButtonUp;
            buttons[input::kMouseRight] = true;
        }

        if (rawMouse.usButtonFlags & RI_MOUSE_MIDDLE_BUTTON_DOWN)
        {
            inputFlags |= input::kButtonDown;
            buttons[input::kMouseMiddle] = true;
        }
        else if (rawMouse.usButtonFlags & RI_MOUSE_MIDDLE_BUTTON_UP)
        {
            inputFlags |= input::kButtonUp;
            buttons[input::kMouseMiddle] = true;
        }

        if (rawMouse.usButtonFlags & RI_MOUSE_BUTTON_4_DOWN)
        {
            inputFlags |= input::kButtonDown;
            buttons[input::kMouse4] = true;
        }
        else if (rawMouse.usButtonFlags & RI_MOUSE_BUTTON_4_UP)
        {
            inputFlags |= input::kButtonUp;
            buttons[input::kMouse4] = true;
        }

        if (rawMouse.usButtonFlags & RI_MOUSE_BUTTON_5_DOWN)
        {
            inputFlags |= input::kButtonDown;
            buttons[input::kMouse5] = true;
        }
        else if (rawMouse.usButtonFlags & RI_MOUSE_BUTTON_5_UP)
        {
            inputFlags |= input::kButtonUp;
            buttons[input::kMouse5] = true;
        }

        if (rawMouse.usButtonFlags & RI_MOUSE_WHEEL)
        {
            int z = 0;
        }
        
        if (inputFlags || !buttons.AllBitsFalse() || !(mousePos == input::InputDevice::Mouse::Location(0, 0)))
        {
            mInputDevice.MouseRecord(inputFlags, buttons, mousePos);
            return 0;
        }
    }
#endif // 0
    return 1;
}

bool WindowApplication::onMessagePump(const time::GameClock& /*gameClock*/)
{
    MSG msg;
    while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
    {
        // translate keystroke messages into the right format
        TranslateMessage(&msg);

        // send the message to the WindowProc function
        DispatchMessage(&msg);

        // check to see if it's time to quit
        if (msg.message == WM_QUIT)
        {
            return false;
        }
    }

    return true;
}

void WindowApplication::Cleanup()
{
    ::DestroyWindow(GetWindowHandle<HWND>());
}


#if 0

switch (virtualKey)
{
// right-hand CONTROL and ALT have their e0 bit set
case VK_CONTROL:
    if (isE0)
        virtualKey = Keys::RIGHT_CONTROL;
    else
        virtualKey = Keys::LEFT_CONTROL;
    break;

case VK_MENU:
    if (isE0)
        virtualKey = Keys::RIGHT_ALT;
    else
        virtualKey = Keys::LEFT_ALT;
    break;

// NUMPAD ENTER has its e0 bit set
case VK_RETURN:
    if (isE0)
        virtualKey = Keys::NUMPAD_ENTER;
    break;

// the standard INSERT, DELETE, HOME, END, PRIOR and NEXT keys will always have their e0 bit set, but the
// corresponding keys on the NUMPAD will not.
case VK_INSERT:
    if (!isE0)
        virtualKey = Keys::NUMPAD_0;
    break;

case VK_DELETE:
    if (!isE0)
        virtualKey = Keys::NUMPAD_DECIMAL;
    break;

case VK_HOME:
    if (!isE0)
        virtualKey = Keys::NUMPAD_7;
    break;

case VK_END:
    if (!isE0)
        virtualKey = Keys::NUMPAD_1;
    break;

case VK_PRIOR:
    if (!isE0)
        virtualKey = Keys::NUMPAD_9;
    break;

case VK_NEXT:
    if (!isE0)
        virtualKey = Keys::NUMPAD_3;
    break;

// the standard arrow keys will always have their e0 bit set, but the
// corresponding keys on the NUMPAD will not.
case VK_LEFT:
    if (!isE0)
        virtualKey = Keys::NUMPAD_4;
    break;

case VK_RIGHT:
    if (!isE0)
        virtualKey = Keys::NUMPAD_6;
    break;

case VK_UP:
    if (!isE0)
        virtualKey = Keys::NUMPAD_8;
    break;

case VK_DOWN:
    if (!isE0)
        virtualKey = Keys::NUMPAD_2;
    break;

// NUMPAD 5 doesn't have its e0 bit set
case VK_CLEAR:
    if (!isE0)
        virtualKey = Keys::NUMPAD_5;
    break;
}
#endif // 0
