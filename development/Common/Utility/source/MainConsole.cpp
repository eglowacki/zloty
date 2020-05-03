#include "App/MainConsole.h"
#include "Config/ConfigHelper.h"
#include "Message/Dispatcher.h"
#include "Timer/Clock.h"
#include "Input/InputManager.h"
#include <boost/bind.hpp>

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>


namespace eg {

#if 0
MainConsole::MainConsole(const std::string& title, const wxPoint& /*pos*/, const wxSize& /*size*/)
: mQuit(false)
, mAppConsole(wxAppConsole::GetInstance())
{
    ::SetConsoleTitle(title.c_str());

    ClockManager& cm = REGISTRATE(ClockManager);
    mClock.reset(new Clock(cm));
}


MainConsole::~MainConsole()
{
}


int MainConsole::Run()
{
    InputActionCallback_t inputCallbackQuit = boost::bind(&MainConsole::InputCallback_Quit, this, _1, _2, _3, _4);
    InputManager::Key *input = new InputManager::Key(InputTypes::kButtonDown, 27);
    InputManager& im = REGISTRATE(InputManager);
    im.RegisterAction("Quit App", input, 0, inputCallbackQuit);

    HANDLE hStdInput = ::GetStdHandle(STD_INPUT_HANDLE);
    INPUT_RECORD inputRec = {0};

    while (!mQuit)
    {
        //TickSystem();
        float deltaTime = static_cast<float>(mClock->GetFrameDuration());
        double currTime = mClock->GetTime();

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
                wxLogError("Could not read input from Console Application.");
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
                    flags |= InputTypes::kButtonDown;
                }

                InputManager& im = REGISTRATE(InputManager);
                int keyValue = im.MapKey(inputRec.Event.KeyEvent.uChar.AsciiChar);
                InputManager::Key *pKey = new InputManager::Key(static_cast<const uint32_t>(currTime * 1000.0f), flags, keyValue);
                im.ProcessInput(pKey);
            }

            break;
        }

        assert(0, "Fix me");
        //Message(mtype::kFrameTick, deltaTime).Send();
        Tick(deltaTime);
    }

    im.UnregisterAction("Quit App");

    //Message(mtype::kShutdownBegin).Send();
    //Message(mtype::kFrameTick, 0.0f).Send();
    return 0;
}


void MainConsole::InputCallback_Quit(const std::string& /*actionName*/, uint32_t /*timeStamp*/, int32_t /*mouseX*/, int32_t /*mouseY*/)
{
    mQuit = true;
}
#endif // 0


//This will clear the console while setting the forground and
//  background colors.
void MainConsole::ClearConsole(int ForgC, int BackC)
{
    WORD wColor = ((BackC & 0x0F) << 4) + (ForgC & 0x0F);
    //Get the handle to the current output buffer...
    HANDLE hStdOut = ::GetStdHandle(STD_OUTPUT_HANDLE);
    //This is used to reset the carat/cursor to the top left.
    COORD coord = {0, 0};
    //A return value... indicating how many chars were written
    //   not used but we need to capture this since it will be
    //   written anyway (passing NULL causes an access violation).
    DWORD count;

    //This is a structure containing all of the console info
    // it is used here to find the size of the console.
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    //Here we will set the current color
    ::SetConsoleTextAttribute(hStdOut, wColor);
    if(::GetConsoleScreenBufferInfo(hStdOut, &csbi))
    {
        //This fills the buffer with a given character (in this case 32=space).
        ::FillConsoleOutputCharacter(hStdOut, (TCHAR) 32, csbi.dwSize.X * csbi.dwSize.Y, coord, &count);

        ::FillConsoleOutputAttribute(hStdOut, csbi.wAttributes, csbi.dwSize.X * csbi.dwSize.Y, coord, &count );
        //This will set our cursor position for the next print statement.
        ::SetConsoleCursorPosition(hStdOut, coord);
    }
    return;
}


//This will clear the console.
void MainConsole::ClearConsole()
{
    //Get the handle to the current output buffer...
    HANDLE hStdOut = ::GetStdHandle(STD_OUTPUT_HANDLE);
    //This is used to reset the carat/cursor to the top left.
    COORD coord = {0, 0};
    //A return value... indicating how many chars were written
    //   not used but we need to capture this since it will be
    //   written anyway (passing NULL causes an access violation).
    DWORD count;
    //This is a structure containing all of the console info
    // it is used here to find the size of the console.
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    //Here we will set the current color
    if(::GetConsoleScreenBufferInfo(hStdOut, &csbi))
    {
        //This fills the buffer with a given character (in this case 32=space).
        ::FillConsoleOutputCharacter(hStdOut, (TCHAR) 32, csbi.dwSize.X * csbi.dwSize.Y, coord, &count);
        ::FillConsoleOutputAttribute(hStdOut, csbi.wAttributes, csbi.dwSize.X * csbi.dwSize.Y, coord, &count );
        //This will set our cursor position for the next print statement.
        ::SetConsoleCursorPosition(hStdOut, coord);
    }
    return;
}


//This will set the position of the cursor
void MainConsole::gotoXY(int x, int y)
{
    //Initialize the coordinates
    COORD coord = {x, y};
    //Set the position
    ::SetConsoleCursorPosition(::GetStdHandle(STD_OUTPUT_HANDLE), coord);
    return;
}


//This will set the forground color for printing in a console window.
void MainConsole::SetColor(int ForgC)
{
    WORD wColor;
    //We will need this handle to get the current background attribute
    HANDLE hStdOut = ::GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO csbi;

    //We use csbi for the wAttributes word.
    if(::GetConsoleScreenBufferInfo(hStdOut, &csbi))
    {
        //Mask out all but the background attribute, and add in the forgournd color
        wColor = (csbi.wAttributes & 0xF0) + (ForgC & 0x0F);
        ::SetConsoleTextAttribute(hStdOut, wColor);
    }
    return;
}


//This will set the forground and background color for printing in a console window.
void MainConsole::SetColor(int ForgC, int BackC)
{
    WORD wColor = ((BackC & 0x0F) << 4) + (ForgC & 0x0F);;
    ::SetConsoleTextAttribute(::GetStdHandle(STD_OUTPUT_HANDLE), wColor);
    return;
}


//Direct console output
void MainConsole::ConPrint(const char *pCharBuffer, int len)
{
    DWORD count;
    ::WriteConsole(::GetStdHandle(STD_OUTPUT_HANDLE), pCharBuffer, len, &count, NULL);
    return;
}


//Direct Console output at a particular coordinate.
void MainConsole::ConPrintAt(int x, int y, const char *pCharBuffer, int len)
{
    DWORD count;
    COORD coord = {x, y};
    HANDLE hStdOut = ::GetStdHandle(STD_OUTPUT_HANDLE);
    ::SetConsoleCursorPosition(hStdOut, coord);
    ::WriteConsole(hStdOut, pCharBuffer, len, &count, NULL);
    return;
}


//Hides the console cursor
void MainConsole::HideCursor()
{
    CONSOLE_CURSOR_INFO cciCursor;
    HANDLE hStdOut = ::GetStdHandle(STD_OUTPUT_HANDLE);

    if(::GetConsoleCursorInfo(hStdOut, &cciCursor))
    {
        cciCursor.bVisible=FALSE;
    }
    return;
}


//Shows the console cursor
void MainConsole::ShowCursor()
{
    CONSOLE_CURSOR_INFO cciCursor;
    HANDLE hStdOut = ::GetStdHandle(STD_OUTPUT_HANDLE);

    if(::GetConsoleCursorInfo(hStdOut, &cciCursor))
    {
        cciCursor.bVisible=TRUE;
    }
    return;
}


/*
void MainConsole::Tick(float deltaTime);

#include "ServerBase.h"
#include "Timer/Clock.h"
#include "Registrate.h"
#include "MessageInterface.h"
#include "BaseDLLs.h"

#include <windows.h>
#include <stdio.h>


void ConPrint(char *CharBuffer, int len);
void ConPrintAt(int x, int y, char *CharBuffer, int len);
void gotoXY(int x, int y);
void ClearConsole();
void ClearConsole(int ForgC, int BackC);
void SetColor(int ForgC, int BackC);
void SetColor(int ForgC);
void HideCursor();
void ShowCursor();


using namespace eg;
int main(int argc, char* argv[])
{
    wxInitializer initializerObject(argc, argv);
    Message(mtype::kAppValid).Send();
    ClockManager& cm = REGISTRATE(ClockManager);
    HideCursor();
    ClearConsole(15, 1);
    ClearConsole();
    gotoXY(1,1);
    SetColor(14);
    printf("This is a test...\n");
    Sleep(1000);
    ShowCursor();
    SetColor(15,12);
    ConPrint("This is also a test...\n", 23);
    SetColor(1, 7);
    ConPrintAt(22, 15, "This is also a test...\n", 23);
    gotoXY(0, 24);
    SetColor(7,1);

    Message(mtype::kShutdownBegin).Send();
    Message(mtype::kShutdownEnd).Send();

    return 0;
}


*/
} // namespace eg
