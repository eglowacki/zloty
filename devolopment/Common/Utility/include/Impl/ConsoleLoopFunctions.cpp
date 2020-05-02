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
        quitApp = true;
    }

} // namespace


namespace eg
{

void ConsoleLoop(InputManager& input, ClockManager& clock, boost::function<void (const Clock&)> callback)
{
    Clock main_time(clock);
    InputManager::ActionListener_t action_quit = input.ListenForAction("App.Quit", onQuit);

    HANDLE hStdInput = ::GetStdHandle(STD_INPUT_HANDLE);
    INPUT_RECORD inputRec = {0};

    MainConsole::ClearConsole();
    // DARK_GRAY
    MainConsole::SetColor(MainConsole::DGray);
    MainConsole::ConPrintAt(0, 0, "ESC to quit.");
    //float timer = 0;

    /*
    for (int i = 0; i < 16; i++)
    {
        MainConsole::SetColor(0, i);
        MainConsole::ConPrintAt(0, i, "[      Test color pattern]");
    }
    */

    MainConsole::SetColor(FOREGROUND_GREEN);
    while (!quitApp)
    {
        Prof(MainLoop);
        uint32_t t = 1;
        util::sleep(t);
        if (callback)
        {
            callback(main_time);
        }

        clock.FrameStep();
        double currTime = main_time.GetTime();

        inputRec.EventType = 0;
        DWORD numRead = 0;
        DWORD numPendingInputs = 0;
        ::GetNumberOfConsoleInputEvents(hStdInput, &numPendingInputs);
        if (numPendingInputs)
        {
            if (::ReadConsoleInput(hStdInput, &inputRec, 1, &numRead))
            {
            }
            else
            {
                log_error << "Could not read input from Console Application.";
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
                uint32_t flags = altPressed ? InputTypes::kButtonAlt : 0;
                flags |= ctrlPressed ? InputTypes::kButtonCtrl : 0;
                flags |= shiftPressed ? InputTypes::kButtonShift : 0;


                if (inputRec.Event.KeyEvent.bKeyDown)
                {
                    flags |= InputTypes::kButtonDown;
                }
                else
                {
                    flags |= InputTypes::kButtonUp;
                }

                int currentKey = inputRec.Event.KeyEvent.uChar.AsciiChar ? inputRec.Event.KeyEvent.uChar.AsciiChar : inputRec.Event.KeyEvent.wVirtualKeyCode;
                int keyValue = input.MapKey(currentKey);
                log_debug << "Input generated, Key: " << keyValue << ", altPressed: " << logs::boolean(altPressed) <<  ", ctrlPressed: " << logs::boolean(ctrlPressed) <<  ", shiftPressed: " << logs::boolean(shiftPressed);
                InputManager::Key *pKey = new InputManager::Key(static_cast<const uint32_t>(currTime * 1000.0f), flags, keyValue);
                input.ProcessInput(pKey);
            }

            break;
        }
    }

}

} // namespace eg


