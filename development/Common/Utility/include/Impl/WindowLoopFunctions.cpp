#include "Timer/Clock.h"
#include "Input/InputManager.h"
#include "App/MainConsole.h"
#include "App/AppUtilities.h"
#include "Message/Dispatcher.h"
#include "Logger/Log.h"
#include "Profiler/Profiler.h"
#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

using namespace eg;
namespace bfs = boost::filesystem;


namespace
{
    bool quitApp = false;
    void onQuit(const std::string& /*actionName*/, uint32_t /*timeStamp*/, int32_t /*mouseX*/, int32_t /*mouseY*/)
    {
        //quitApp = true;
        PostQuitMessage(0);
    }


    struct Globals
    {
        Globals() : input(0), clock(0), focus(false), minimized(false) {}

        InputManager *input;
        Clock *clock;
        bool focus;
        bool minimized;
    };


} // namespace


LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam);

namespace eg
{
    const char *kWinMain = "YAGET.MAIN";

    /*
    void RegisterWindowTypes(HINSTANCE hInstance)
    {
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
        wc.lpfnWndProc = WndProc;
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
    }
    */

    void WindowLoop(InputManager& input, ClockManager& clockMananger, boost::function<void (const Clock&)> callback, HINSTANCE hInstance, int iCmdShow)
    {
        Clock main_time(clockMananger);
        InputManager::ActionListener_t action_quit = input.ListenForAction("App.Quit", onQuit);
        Globals globals;
        globals.input = &input;
        globals.clock = &main_time;

        HWND hwnd = CreateWindow(TEXT(kWinMain), TEXT("Win"), WS_OVERLAPPEDWINDOW, 10, 10, 200, 200, NULL, NULL, hInstance, NULL);
        SetWindowLong(hwnd, GWL_USERDATA, reinterpret_cast<long>(&globals));

        ShowWindow(hwnd, iCmdShow);
        UpdateWindow(hwnd);

        /*
        MSG msg;
        while (!quitApp)
        {
            Prof(MainLoop);
            if (!globals.focus)
            {
                uint32_t t = 1;
                util::sleep(t);
            }

            if (callback)
            {
                callback(main_time);
            }
            clockMananger.FrameStep();

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
        }
        */
    }


} // namespace


#define M_BIT(n) (1 << n)

LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam)
{
    Globals *globals = reinterpret_cast<Globals *>(GetWindowLong(hwnd, GWL_USERDATA));
    InputManager *input = globals ? globals->input : 0;
    Clock *clock = globals ? globals->clock : 0;

    switch( message )
    {
    case WM_CREATE:
        return 0;

    case WM_PAINT:
    {
        // we would place our Windows painting code here.
        HDC hdc;
        PAINTSTRUCT ps;
        hdc = BeginPaint(hwnd, &ps);
        EndPaint( hwnd, &ps );
    }
    break;

    case WM_KEYUP:
        if (input)
        {
            //log_trace("main") << "WM_KEYUP WPARAM: " << wparam << ", LPARAM: " << lparam;
            double currTime = clock->GetTime();
            uint32_t flags = InputTypes::kButtonUp;

            int keyValue = input->MapKey(wparam);
            InputManager::Key *pKey = new InputManager::Key(static_cast<const uint32_t>(currTime * 1000.0f), flags, keyValue);
            input->ProcessInput(pKey);
        }
        return 0;

    case WM_KEYDOWN:
        if (input)
        {
            double currTime = clock->GetTime();
            bool altPressed = GetAsyncKeyState(VK_MENU);
            bool ctrlPressed = GetAsyncKeyState(VK_CONTROL);
            bool shiftPressed = GetAsyncKeyState(VK_SHIFT);

            if (!(lparam & M_BIT(30)))
            {
                //log_trace("main") << "WM_KEYDOWN WPARAM: " << wparam << ", LPARAM: " << lparam << ", Alt: " << logs::boolean(altPressed) << ", Ctrl: " << logs::boolean(ctrlPressed) << ", Shift: " << logs::boolean(shiftPressed);
                uint32_t flags = altPressed ? InputTypes::kButtonAlt : 0;
                flags |= ctrlPressed ? InputTypes::kButtonCtrl : 0;
                flags |= shiftPressed ? InputTypes::kButtonShift : 0;
                flags |= InputTypes::kButtonDown;

                int keyValue = input->MapKey(wparam);
                InputManager::Key *pKey = new InputManager::Key(static_cast<const uint32_t>(currTime * 1000.0f), flags, keyValue);
                input->ProcessInput(pKey);
            }
        }
        return 0;

    case WM_ACTIVATE:
    {
        uint32_t active = LOWORD(wparam);
        BOOL minimized = (BOOL)HIWORD(wparam);
        if (globals)
        {
            globals->focus = active != WA_INACTIVE;
            globals->minimized = minimized == TRUE;
        }
    }
    break;

    case WM_DESTROY:
        PostQuitMessage(0) ;
        return 0;
    }

    return DefWindowProc(hwnd, message, wparam, lparam);
}

