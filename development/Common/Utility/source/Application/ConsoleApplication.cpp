#include "App/ConsoleApplication.h"
#include "Logger/YLog.h"
#include "Platform/WindowsLean.h"
#include "Input/InputDevice.h"

using namespace yaget;

namespace
{
    BOOL WINAPI HandlerRoutine(DWORD dwCtrlType)
    {
        return dwCtrlType == CTRL_CLOSE_EVENT ? TRUE : FALSE;
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

math3d::Vector2 app::BlankApplication::GetWindowSize() const
{
    return { 32, 32 };
}

void app::BlankApplication::Cleanup()
{
}


bool app::BlankApplication::onMessagePump(const time::GameClock& /*gameClock*/)
{
    return true;
}

ConsoleApplication::ConsoleApplication(const std::string& title, items::Director& director, io::VirtualTransportSystem& vts, const args::Options& options)
    : Application(title, director, vts, options)
{
    ::SetConsoleTitle(title.c_str());
    ::SetConsoleCtrlHandler(HandlerRoutine, TRUE);
    mWinHandle = ::GetStdHandle(STD_INPUT_HANDLE);
    mOutputHandle = ::GetStdHandle(STD_OUTPUT_HANDLE);
}

ConsoleApplication::~ConsoleApplication()
{}

bool ConsoleApplication::onMessagePump(const time::GameClock& /*gameClock*/)
{
    INPUT_RECORD inputRec = {0};

    inputRec.EventType = 0;
    DWORD numRead = 0;
    DWORD numPendingInputs = 0;
    ::GetNumberOfConsoleInputEvents(mWinHandle, &numPendingInputs);
    if (numPendingInputs)
    {
        if (::ReadConsoleInput(mWinHandle, &inputRec, 1, &numRead))
        { }
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

math3d::Vector2 ConsoleApplication::GetWindowSize() const
{
    return {1, 1};
}


#if  0
void ConsoleApplication::Clear(int ForgC /*= -1*/, int BackC /*= -1*/)
{
    if (ForgC != -1 && BackC != -1)
    {
        WORD wColor = ((BackC & 0x0F) << 4) + (ForgC & 0x0F);
        ::SetConsoleTextAttribute(mOutputHandle, wColor);
    }

    CONSOLE_SCREEN_BUFFER_INFO csbi;
    if (GetConsoleScreenBufferInfo(mOutputHandle, &csbi))
    {
        COORD coord = { 0, 0 };
        DWORD count = 0;
        ::FillConsoleOutputCharacter(mOutputHandle, static_cast<TCHAR>(32), csbi.dwSize.X * csbi.dwSize.Y, coord, &count);
        ::FillConsoleOutputAttribute(mOutputHandle, csbi.wAttributes, csbi.dwSize.X * csbi.dwSize.Y, coord, &count);
        ::SetConsoleCursorPosition(mOutputHandle, coord);
    }
}

void ConsoleApplication::SetColor(int ForgC, int BackC /*= -1*/)
{
    WORD wColor = 0;
    if (BackC == -1)
    {
        CONSOLE_SCREEN_BUFFER_INFO csbi;
        if (::GetConsoleScreenBufferInfo(mOutputHandle, &csbi))
        {
            wColor = (csbi.wAttributes & 0xF0) + (ForgC & 0x0F);
        }
    }
    else
    {
        wColor = ((BackC & 0x0F) << 4) + (ForgC & 0x0F);;
    }

    ::SetConsoleTextAttribute(mOutputHandle, wColor);
}

void ConsoleApplication::SetColor(float /*r*/, float /*g*/, float /*b*/, float /*a*/)
{
}

void ConsoleApplication::Print(const std::string& charBuffer)
{
    DWORD count;
    ::WriteConsole(mOutputHandle, charBuffer.c_str(), static_cast<DWORD>(charBuffer.size()), &count, nullptr);
}

void ConsoleApplication::PrintAt(int x, int y, const std::string& charBuffer)
{
    COORD coord = { static_cast<SHORT>(x), static_cast<SHORT>(y) };
    ::SetConsoleCursorPosition(mOutputHandle, coord);
    Print(charBuffer);
}

void ConsoleApplication::GotoXY(int x, int y)
{
    COORD coord = {static_cast<SHORT>(x), static_cast<SHORT>(y)};
    ::SetConsoleCursorPosition(mOutputHandle, coord);
}

void ConsoleApplication::Cursor(bool bShow)
{
    CONSOLE_CURSOR_INFO cciCursor;
    if (::GetConsoleCursorInfo(mOutputHandle, &cciCursor))
    {
        cciCursor.bVisible = bShow;
    }
}

void ConsoleApplication::SetCursorLoc(int /*x*/, int /*y*/)
{
}
#endif // 0

app::DefaultConsole::DefaultConsole(const std::string& title, items::Director& director, io::VirtualTransportSystem& vts, const args::Options& options)
    : ConsoleApplication(title, director, vts, options)
{
    Input().RegisterSimpleActionCallback("Quit App", [this]()
    {
        YLOG_INFO("APP", "exit requested...");
        RequestQuit();
    });
}
