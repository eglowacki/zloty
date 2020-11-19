#include "App/ConsoleApplication.h"
#include "Logger/YLog.h"
#include "Platform/WindowsLean.h"
#include "Input/InputDevice.h"

using namespace yaget;

namespace
{
    // NOTE: EG total unhappiness with this approach. If we do not request quit 
    // windows will simply kill our app, hence this (g)lobal variable
    yaget::Application* gConsoleApplication = nullptr;
    BOOL WINAPI HandlerRoutine(DWORD dwCtrlType)
    {
        const bool closeApp = dwCtrlType == CTRL_CLOSE_EVENT || dwCtrlType == CTRL_C_EVENT || dwCtrlType == CTRL_BREAK_EVENT || dwCtrlType == CTRL_LOGOFF_EVENT || dwCtrlType == CTRL_SHUTDOWN_EVENT;

        if (closeApp && gConsoleApplication)
        {
            gConsoleApplication->RequestQuit();
            return TRUE;
        }
        return FALSE;
    }

    HANDLE handle_cast(void* handle)
    {
        return reinterpret_cast<HANDLE>(handle);
    }
} // namespace


app::BlankApplication::BlankApplication(const std::string& title, items::Director& director, io::VirtualTransportSystem& vts, const args::Options& options)
    : Application(title, director, vts, options)
{
}

void app::BlankApplication::Cleanup()
{
}

bool app::BlankApplication::onMessagePump(const time::GameClock& /*gameClock*/)
{
    return !mQuit;
}

ConsoleApplication::ConsoleApplication(const std::string& title, items::Director& director, io::VirtualTransportSystem& vts, const args::Options& options)
    : Application(title, director, vts, options)
{
    ::SetConsoleTitle(title.c_str());

    gConsoleApplication = this;
    ::SetConsoleCtrlHandler(HandlerRoutine, TRUE);
    mOutputHandle = ::GetStdHandle(STD_OUTPUT_HANDLE);
    mInputHandle = ::GetStdHandle(STD_INPUT_HANDLE);
}

ConsoleApplication::~ConsoleApplication()
{
    gConsoleApplication = nullptr;
}

bool ConsoleApplication::onMessagePump(const time::GameClock& /*gameClock*/)
{
    INPUT_RECORD inputRec = {0};

    inputRec.EventType = 0;
    DWORD numRead = 0;
    DWORD numPendingInputs = 0;
    ::GetNumberOfConsoleInputEvents(mInputHandle, &numPendingInputs);
    if (numPendingInputs)
    {
        if (::ReadConsoleInput(mInputHandle, &inputRec, 1, &numRead))
        {
        }
        else
        {
            //wxLogError("Could not read input from Console Application.");
        }
    }

    switch (inputRec.EventType)
    {
        case KEY_EVENT:
        {
            bool altPressed = (inputRec.Event.KeyEvent.dwControlKeyState & LEFT_ALT_PRESSED) ? true : false;
            if (inputRec.Event.KeyEvent.dwControlKeyState & RIGHT_ALT_PRESSED)
            {
                altPressed = true;
            }

            bool ctrlPressed = (inputRec.Event.KeyEvent.dwControlKeyState & LEFT_CTRL_PRESSED) ? true : false;
            if (inputRec.Event.KeyEvent.dwControlKeyState & RIGHT_CTRL_PRESSED)
            {
                ctrlPressed = true;
            }

            bool shiftPressed = (inputRec.Event.KeyEvent.dwControlKeyState & SHIFT_PRESSED) ? true : false;
            uint32_t flags = altPressed ? input::kButtonAlt : 0;
            flags |= ctrlPressed ? input::kButtonCtrl : 0;
            flags |= shiftPressed ? input::kButtonShift : 0;
            int keyValue = mInputDevice.MapKey(inputRec.Event.KeyEvent.wVirtualKeyCode);

            if (inputRec.Event.KeyEvent.bKeyDown)
            {
                if (!mLastKeyState[keyValue])
                {
                    mLastKeyState[keyValue] = true;
                    flags |= input::kButtonDown;
                }
                else
                {
                    // skip this key
                    break;
                }
            }
            else
            {
                mLastKeyState[keyValue] = false;
                flags |= input::kButtonUp;
            }

            YLOG_DEBUG("SPAM", "Pressed key is: '%d', with flags: '%d'", keyValue, flags);
            mInputDevice.KeyRecord(flags, keyValue);
        }
        break;
    }

    return !mQuit;
}

void ConsoleApplication::Cleanup()
{
}

app::DefaultConsole::DefaultConsole(const std::string& title, items::Director& director, io::VirtualTransportSystem& vts, const args::Options& options)
    : ConsoleApplication(title, director, vts, options)
{
    Input().RegisterSimpleActionCallback("Quit App", [this]()
    {
        YLOG_INFO("APP", "exit requested...");
        RequestQuit();
    });
}

app::DefaultConsole::~DefaultConsole() = default;
