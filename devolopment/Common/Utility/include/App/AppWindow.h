/////////////////////////////////////////////////////////////////////////
// AppWindow.h
//
//  Copyright 4/29/2009 Edgar Glowacki.
//
// NOTES:
//      This is purly win32 implementation and as such includes windows header.
//
//
// #include "App/AppWindow.h"
//
/////////////////////////////////////////////////////////////////////////
//! \file

#ifndef APP_APP_WINDOW_H
#define APP_APP_WINDOW_H
#pragma once

#include "App/AppContext.h"
#include "App/AppUtilities.h"
#include "Exception/Exception.h"
#include "Profiler/Profiler.h"
#include "Shiny.h"
#include <list>
#include <boost/foreach.hpp>
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>


namespace eg
{

    namespace app
    {
        static const char *kWinMain = "YAGET.MAIN";

        inline long to_long(HWND handle)
        {
            return reinterpret_cast<long>(handle);
        }

        inline HWND to_handle(long handle)
        {
            return reinterpret_cast<HWND>(handle);
        }

        class Window
        {
            struct Settings
            {
                Settings() : x(CW_USEDEFAULT), y(CW_USEDEFAULT), w(CW_USEDEFAULT), h(CW_USEDEFAULT), flags(WS_OVERLAPPEDWINDOW)
                {}

                int x, y, w, h;
                uint32_t flags;
            };

            friend std::ostream &operator<<(std::ostream &stream, const Settings& ob);
            friend std::istream &operator>>(std::istream &stream, Settings &ob);

        public:
            Window(const std::string& name)
            : mTime(Window::Context->clockManager)
            , mMinimized(false)
            , mFocus(false)
            , mHandle(0)
            , mName(name)
            , mRootWindow(Window::Windows.empty())
            {
                // let's see if we have configuration saved for this window
                //Settings uiSettings;
                VirtualFileFactory::istream_t stream = Window::Context->vfs.GetFileStream(normalize(mName));
                if (stream)
                {
                    *stream >> mUiSettings;
                }

                mHandle = CreateWindow(TEXT(kWinMain), mName.c_str(), WS_OVERLAPPEDWINDOW, mUiSettings.x, mUiSettings.y, mUiSettings.w, mUiSettings.h, NULL, NULL, Window::Instance, NULL);
                if (!mHandle)
                {
                    throw ex::program("Could not create Window (win32)");
                }

                SetWindowLong(mHandle, GWL_USERDATA, reinterpret_cast<long>(this));
                Window::Windows.push_back(this);
            }

            virtual ~Window()
            {
                log_trace("main") << "Destroying Window: '" << mName << "'.";
                std::remove(Window::Windows.begin(), Window::Windows.end(), this);
            }

            long Handle() const {return to_long(mHandle);}

            void Show(bool show)
            {
                ShowWindow(mHandle, (show ? SW_SHOW : SW_HIDE));
                if (show)
                {
                    UpdateWindow(mHandle);
                }
            }

            void Close()
            {
                if (mHandle)
                {
                    PostMessage(mHandle, WM_CLOSE, 0, 0);
                }
                else
                {
                    log_warning << "Calling Close on Window '" << mName << "' when handle is 0";
                }
            }

            static void Run(boost::function<void (const Clock&)> callback)
            {
                Clock mainTime(Window::Context->clockManager);
                InputManager::ActionListener_t action_quit = Window::Context->input.ListenForAction("App.Quit", Window::onQuit);

                MSG msg;
                bool quitApp = false;
                while (!quitApp)
                {
                    //Prof(MainLoop);
                    if (!Window::Active)
                    {
                        uint32_t t = 10;
                        util::sleep(t);
                    }

                    Window::Context->clockManager.FrameStep();
                    float deltaTime = static_cast<float>(mainTime.GetFrameDuration());

                    Window::Context->input.Tick(deltaTime);

                    if (callback)
                    {
                        callback(mainTime);
                    }

                    //log_trace("main") << "Delta: " << deltaTime;
                    Window::Context->dispatcher[mtype::kFrameTick](Message(mtype::kFrameTick, deltaTime));

                    while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
                    {
                        if (msg.message == WM_QUIT)
                        {
                            quitApp = true;
                            break;
                        }

                        TranslateMessage(&msg);
                        DispatchMessage(&msg);
                    }

                    //Window::Context->profiler.Tick(deltaTime);
                    //PROFILE_UPDATE_ALL(); // update all profiles
                    //PROFILE_OUTPUT_ALL(); // print to cout
                }
            }

            static void Initialize(HINSTANCE hInstance, Context *context)
            {
                if (!context)
                {
                    log_error << "There is no valid context for Window Initialization";
                    //throw ex::program("There is no valid context for Window Initialization");
                    return;
                }
                WNDCLASS wc;
                wc.cbClsExtra = 0;  // ignore for now
                wc.cbWndExtra = 0;  // ignore for now
                // I want the window to have a white background
                wc.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
                // I want it to have an arrow for a cursor
                wc.hCursor = LoadCursor(NULL, IDC_ARROW);
                // I want it to have that envelope like icon
                wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
                // INSTANCE HANDLE -- see the GLOSSARY PART of this file for an explanation of what HINSTANCE is
                wc.hInstance = hInstance;
                // Give name of WndProc function here.
                wc.lpfnWndProc = Window::WndProc;
                // I have named it Philip.
                wc.lpszClassName = TEXT(kWinMain);
                // You could name it anything
                // you want, but you have to
                // remember the name for when
                // you call CreateWindow().
                wc.lpszMenuName = 0;    // no menu - ignore
                wc.style = CS_HREDRAW | CS_VREDRAW; // Redraw the window
                // on BOTH horizontal resizes (CS_HREDRAW) and
                // vertical resizes (CS_VREDRAW).

                RegisterClass(&wc);

                Window::Instance = hInstance;
                Window::Context = context;
            }


        private:
            #define M_BIT(n) (1 << n)

            static Vector2 mousePos(LPARAM lparam)
            {
                Vector2 pos;
                pos.x = static_cast<float>(LOWORD(lparam));
                pos.y = static_cast<float>(HIWORD(lparam));
                return pos;
            }

            LRESULT procFunction(UINT message, WPARAM wparam, LPARAM lparam)
            {
                switch( message )
                {
                case WM_CREATE:
                    log_trace("main") << "Window '" << mName << " - WM_CREATE wparam '" << wparam;
                    break;

                case WM_SHOWWINDOW:
                    log_trace("main") << "Window '" << mName << " - WM_SHOWWINDOW wparam '" << logs::boolean(wparam == TRUE);
                    break;

                case WM_PAINT:
                {
                    // we would place our Windows painting code here.
                    HDC hdc;
                    PAINTSTRUCT ps;
                    hdc = BeginPaint(mHandle, &ps);
                    EndPaint(mHandle, &ps);
                    break;
                }
                case WM_MOUSEMOVE:
                {
                    double currTime = mTime.GetTime();
                    bool altPressed = (GetAsyncKeyState(VK_MENU) & 0x8000) != 0;
                    bool ctrlPressed = (GetAsyncKeyState(VK_CONTROL) & 0x8000) != 0;
                    bool shiftPressed = (GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0;
                    uint32_t flags = altPressed ? InputTypes::kButtonAlt : 0;
                    flags |= ctrlPressed ? InputTypes::kButtonCtrl : 0;
                    flags |= shiftPressed ? InputTypes::kButtonShift : 0;
                    flags |= InputTypes::kMouseMove;

                    InputManager::Mouse *pMouse = new InputManager::Mouse(static_cast<uint32_t>(currTime * 1000.0f), flags,  mousePos(lparam));
                    pMouse->mUser = to_long(mHandle);
                    if (wparam & MK_LBUTTON)
                    {
                        pMouse->mButtons[InputTypes::kMouseLeft] = true;
                    }
                    if (wparam & MK_RBUTTON)
                    {
                        pMouse->mButtons[InputTypes::kMouseRight] = true;
                    }
                    if (wparam & MK_MBUTTON)
                    {
                        pMouse->mButtons[InputTypes::kMouseMiddle] = true;
                    }
                    Window::Context->input.ProcessInput(pMouse);
                    break;
                }
                case WM_LBUTTONDOWN:
                {
                    ::SetCapture(mHandle);

                    double currTime = mTime.GetTime();
                    bool altPressed = (GetAsyncKeyState(VK_MENU) & 0x8000) != 0;
                    bool ctrlPressed = (GetAsyncKeyState(VK_CONTROL) & 0x8000) != 0;
                    bool shiftPressed = (GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0;
                    uint32_t flags = altPressed ? InputTypes::kButtonAlt : 0;
                    flags |= ctrlPressed ? InputTypes::kButtonCtrl : 0;
                    flags |= shiftPressed ? InputTypes::kButtonShift : 0;
                    flags |= InputTypes::kButtonDown;

                    InputManager::Mouse *pMouse = new InputManager::Mouse(static_cast<uint32_t>(currTime * 1000.0f), flags, mousePos(lparam));
                    pMouse->mUser = to_long(mHandle);
                    pMouse->mButtons[InputTypes::kMouseLeft] = true;
                    Window::Context->input.ProcessInput(pMouse);
                    break;
                }
                case WM_LBUTTONUP:
                {
                    ::ReleaseCapture();

                    double currTime = mTime.GetTime();
                    bool altPressed = (GetAsyncKeyState(VK_MENU) & 0x8000) != 0;
                    bool ctrlPressed = (GetAsyncKeyState(VK_CONTROL) & 0x8000) != 0;
                    bool shiftPressed = (GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0;
                    uint32_t flags = altPressed ? InputTypes::kButtonAlt : 0;
                    flags |= ctrlPressed ? InputTypes::kButtonCtrl : 0;
                    flags |= shiftPressed ? InputTypes::kButtonShift : 0;
                    flags |= InputTypes::kButtonUp;

                    InputManager::Mouse *pMouse = new InputManager::Mouse(static_cast<uint32_t>(currTime * 1000.0f), flags,  mousePos(lparam));
                    pMouse->mUser = to_long(mHandle);
                    pMouse->mButtons[InputTypes::kMouseLeft] = true;
                    Window::Context->input.ProcessInput(pMouse);
                    break;
                }
                case WM_KEYUP:
                {
                    double currTime = mTime.GetTime();
                    bool altPressed = (GetAsyncKeyState(VK_MENU) & 0x8000) != 0;
                    bool ctrlPressed = (GetAsyncKeyState(VK_CONTROL) & 0x8000) != 0;
                    bool shiftPressed = (GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0;
                    uint32_t flags = altPressed ? InputTypes::kButtonAlt : 0;
                    flags |= ctrlPressed ? InputTypes::kButtonCtrl : 0;
                    flags |= shiftPressed ? InputTypes::kButtonShift : 0;
                    flags |= InputTypes::kButtonUp;

                    int keyValue = Window::Context->input.MapKey(wparam);
                    InputManager::Key *pKey = new InputManager::Key(static_cast<uint32_t>(currTime * 1000.0f), flags, static_cast<unsigned char>(keyValue));
                    pKey->mUser = to_long(mHandle);
                    Window::Context->input.ProcessInput(pKey);
                    break;
                }
                case WM_KEYDOWN:
                {
                    double currTime = mTime.GetTime();
                    bool altPressed = (GetAsyncKeyState(VK_MENU) & 0x8000) != 0;
                    bool ctrlPressed = (GetAsyncKeyState(VK_CONTROL) & 0x8000) != 0;
                    bool shiftPressed = (GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0;

                    if (!(lparam & M_BIT(30)))
                    {
                        uint32_t flags = altPressed ? InputTypes::kButtonAlt : 0;
                        flags |= ctrlPressed ? InputTypes::kButtonCtrl : 0;
                        flags |= shiftPressed ? InputTypes::kButtonShift : 0;
                        flags |= InputTypes::kButtonDown;

                        int keyValue = Window::Context->input.MapKey(wparam);
                        InputManager::Key *pKey = new InputManager::Key(static_cast<uint32_t>(currTime * 1000.0f), flags, static_cast<unsigned char>(keyValue));
                        pKey->mUser = to_long(mHandle);
                        Window::Context->input.ProcessInput(pKey);
                    }
                    break;
                }
                case WM_ACTIVATE:
                {
                    uint32_t active = LOWORD(wparam);
                    BOOL minimized = (BOOL)HIWORD(wparam);
                    mFocus = active != WA_INACTIVE;
                    mMinimized = minimized == TRUE;
                    log_trace("main") << "Window '" << mName << "' - Focus: " << logs::boolean(mFocus) << ", Minimized: " << logs::boolean(mMinimized);
                    break;
                }
                case WM_ACTIVATEAPP:
                    if (mRootWindow)
                    {
                        Window::Active = wparam == TRUE;
                        log_trace("main") << "Window '" << mName << "' - WM_ACTIVATEAPP: " << logs::boolean(Window::Active);
                    }
                    break;
                case WM_DESTROY:
                {
                    if (VirtualFileFactory::ostream_t stream = Window::Context->vfs.AttachFileStream(normalize(mName)))
                    {
                        WINDOWINFO wi;
                        wi.cbSize = sizeof(wi);
                        if (GetWindowInfo(mHandle, &wi))
                        {
                            mUiSettings.x = wi.rcWindow.left;
                            mUiSettings.y = wi.rcWindow.top;
                            mUiSettings.w = wi.rcWindow.right - wi.rcWindow.left;
                            mUiSettings.h = wi.rcWindow.bottom - wi.rcWindow.top;
                            log_trace("main") << "Window '" << mName << " - WM_DESTROY " << mUiSettings;
                            *stream << mUiSettings;
                        }
                    }

                    // let's send message that we are closing
                    if (mRootWindow)
                    {
                        BOOST_FOREACH(Window *win, Window::Windows)
                        {
                            if (win != this)
                            {
                                win->Close();
                            }
                        }
                        PostQuitMessage(0);
                    }

                    mHandle = 0;
                    break;
                }
                case WM_ENTERSIZEMOVE:
                    beginSize();
                    return DefWindowProc(mHandle, message, wparam, lparam);

                case WM_EXITSIZEMOVE:
                    {
                        endSize();

                        RECT rect;
                        GetClientRect(mHandle, &rect);
                        log_trace("main") << "Width: " << rect.right << ", Height: " << rect.bottom << ", Aspect Ratio: " << static_cast<float>(rect.right)/rect.bottom;

                        return DefWindowProc(mHandle, message, wparam, lparam);
                    }

                case WM_SIZING:
                    onSizing();
                    return DefWindowProc(mHandle, message, wparam, lparam);

                case WM_SIZE:
                    //uint32_t w = LOWORD(lparam);
                    //uint32_t h = HIWORD(lparam);
                    //log_trace("main") << "Window '" << mName << " - WM_SIZE wparam width = " << w << ", height = " << h;
                    onSizing();
                    return DefWindowProc(mHandle, message, wparam, lparam);

                case WM_CLOSE:
                    log_trace("main") << "Window '" << mName << " - WM_CLOSE wparam '" << logs::boolean(wparam == TRUE);
                    onClose();
                    return DefWindowProc(mHandle, message, wparam, lparam);

                default:
                    return DefWindowProc(mHandle, message, wparam, lparam);
                }

                return 0;
            }

            static LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam)
            {
                if (Window *This = reinterpret_cast<Window *>(GetWindowLong(hwnd, GWL_USERDATA)))
                {
                    return This->procFunction(message, wparam, lparam);
                }
                else
                {
                    return DefWindowProc(hwnd, message, wparam, lparam);
                }
            }

            static void onQuit(const std::string& /*actionName*/, uint32_t /*timeStamp*/, int32_t /*mouseX*/, int32_t /*mouseY*/)
            {
                /*
                if (Window::Windows.empty())
                {
                    PostQuitMessage(0);
                }
                else
                {
                    Window *window = *Window::Windows.begin();
                    DestroyWindow(window->mHandle);
                }
                */
            }

            static std::string normalize(const std::string& name)
            {
                std::string results = name;
                boost::to_lower(results);
                boost::replace_all(results, " ", "_");
                results += ".ui";

                return results;
            }

            static HINSTANCE Instance;
            static Context *Context;
            static std::list<Window *> Windows;
            static bool Active;

            void beginSize() {}
            void endSize() {onSize();}

            virtual void onClose() {}
            virtual void onSize() {}
            virtual void onSizing() {}

            Clock mTime;
            bool mMinimized;
            bool mFocus;
            HWND mHandle;
            std::string mName;
            Settings mUiSettings;
            bool mRootWindow;
        };

        //! Add this lines in one of your cpp files
        //HINSTANCE app::Window::Instance = 0;
        //app::Context *app::Window::Context = 0;
        //std::list<app::Window *> app::Window::Windows;
        //bool app::Window::Active = false;

        inline std::ostream &operator<<(std::ostream& stream, const Window::Settings& ob)
        {
           stream << ob.x << ' ' << ob.y << ' ' << ob.w << ' ' << ob.h << ' ' << ob.flags << ' ';
           return stream;
        }

        inline std::istream &operator>>(std::istream &stream, Window::Settings &ob)
        {
            stream >> ob.x >> ob.y >> ob.w >> ob.h >> ob.flags;
            return stream;
        }

    } // namespace app


} // namespace eg

#endif // APP_APP_WINDOW_H

