#include "App/WindowApplication.h"
#include "App/AppUtilities.h"
#include "Platform/Support.h"
#include "Platform/WindowsLean.h"
#include "VTS/VirtualTransportSystem.h"
#include "VTS/ResolvedAssets.h"
#include "Debugging/DevConfiguration.h"
#include "Json/JsonHelpers.h"
#include "MathFacade.h"
#include "App/ProcHandler.h"

#include <Windowsx.h>
#include <shellscalingapi.h>


using namespace yaget;
using namespace DirectX;

namespace
{
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

} // namespace


int64_t WindowApplication::_onHandleInputMessage(WindowHandle_t hWnd, uint32_t message, uint64_t wParam, int64_t lParam)
{
    return onHandleRawInput(hWnd, message, wParam, lParam);
}

void WindowApplication::_onSuspend(bool /*bSuspend*/)
{
}

void WindowApplication::ProcessResize()
{
    const auto& requestedSurfaceState = GetSurface().State();
    if (requestedSurfaceState != mActiveSurfaceState)
    {
        mActiveSurfaceState = requestedSurfaceState;

        OnSurfaceStateChange();
    }
    else
    {
        OnResize();
    }
}

app::DisplaySurface WindowApplication::GetSurface() const
{
    const auto& surface = mWindowHandler->ActiveAppearance() == app::Appearance::Fullscreen ? app::SurfaceState::Exclusive : app::SurfaceState::Shared;
    return app::DisplaySurface{ mWindowHandler->WinHandle(), surface };
}

WindowApplication::WindowApplication(const std::string& title, items::Director& director, io::VirtualTransportSystem& vts, const args::Options& options)
    : Application(title, director, vts, options)
    , mWindowHandler(std::make_unique<app::ProcHandler>(
        dev::CurrentConfiguration().mInit,
        title,
        vts, 
        [this](auto&&... params) { return ProcessUserInput(params...); },
        [this]() { ProcessResize(); },
        [this]() { RequestQuit(); return true; }))
    , mActiveSurfaceState(mWindowHandler->ActiveAppearance() == app::Appearance::Fullscreen ? app::SurfaceState::Exclusive : app::SurfaceState::Shared)
{
    RAWINPUTDEVICE Rid[1];

    Rid[0].usUsagePage = 0x01;
    Rid[0].usUsage = 0x06;
    Rid[0].dwFlags = 0;// RIDEV_NOLEGACY;    // adds HID keyboard and also ignores legacy mouse messages
    Rid[0].hwndTarget = mWindowHandler->WinHandle();

    //Rid[1].usUsagePage = 0x01;
    //Rid[1].usUsage = 0x02;
    //Rid[1].dwFlags = RIDEV_INPUTSINK;// RIDEV_INPUTSINK;// RIDEV_NOLEGACY;   // adds HID mouse and also ignores legacy mouse messages
    //Rid[1].hwndTarget = winH;

    ::RegisterRawInputDevices(Rid, 1, sizeof(Rid[0]));
}

WindowApplication::~WindowApplication()
{ 
}

int64_t yaget::WindowApplication::ProcessUserInput(uint32_t message, uint64_t wParam, int64_t lParam)
{
    switch (message)
    {
        case WM_LBUTTONDOWN:
        case WM_LBUTTONUP:
        case WM_MBUTTONDOWN:
        case WM_MBUTTONUP:
        case WM_RBUTTONDOWN:
        case WM_RBUTTONUP:
        case WM_MOUSEMOVE:
        case WM_MOUSEWHEEL:
        {
            int64_t result = _onHandleInputMessage(GetSurface().Handle<HWND>(), message, wParam, lParam);
            if (result)
            {
                return 0;
            }
            else if (_processMouseMessage(message, wParam, lParam) == 0)
            {
                return 0;
            }

            break;
        }

        case WM_INPUT:
        case WM_KEYDOWN:
        case WM_SYSKEYDOWN:
        case WM_KEYUP:
        case WM_CHAR:
            if (message == WM_INPUT)
            {
                if (_processInputMessage(lParam) == 0)
                {
                    return 0;
                }
            }
            else
            {
                int64_t result = _onHandleInputMessage(GetSurface().Handle<HWND>(), message, wParam, lParam);
                if (result)
                {
                    return 0;
                }
            }
            break;
    }

    return 0;
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

    return 1;
}

bool WindowApplication::onMessagePump(const time::GameClock& /*gameClock*/)
{
    // NOTE: This is the spot where we can process any requested events, which are one shot only

    RequestedEvents events;
    {
        std::lock_guard<std::mutex> locker(mEventsMutex);
        std::swap(mRequestedEvents, events);
    }

    while(!events.empty())
    {
        auto& e = events.front();
        e();
        events.pop();
    }

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

void WindowApplication::AddEvent(Event event)
{
    std::lock_guard<std::mutex> locker(mEventsMutex);
    mRequestedEvents.push(std::move(event));
}

void WindowApplication::Cleanup()
{
    ::DestroyWindow(GetSurface().Handle<HWND>());
}
