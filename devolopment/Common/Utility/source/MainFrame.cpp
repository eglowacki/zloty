#if 0
#pragma warning(push)
#pragma warning (disable : 4244)  // '' : conversion from 'int' to 'unsigned short', possible loss of data
#pragma warning (disable : 4512)  // '' : assignment operator could not be generated
#include "App/MainFrame.h"
#include "App/AppTraits.h"
#include "Message/Dispatcher.h"
#include "MessageInterface.h"
#include "IEditorFrame.h"
#include "Timer/Clock.h"
#include "IRenderer.h"
#include "Input/InputManager.h"
#include "Config/Console.h"
#include "Profiler/prof.h"
#include <boost/bind.hpp>
#pragma warning(pop)


namespace
{
    uint32_t getTimeStamp(eg::ClockManager& clockSource)
    {
        return static_cast<uint32_t>(clockSource.GetRealTime() * 1000);
    }

} // namespace


namespace eg {

boost::shared_ptr<std::ofstream> AppTraits::msFileStream;


wxAppTraits *MainApp::CreateTraits()
{
    return new AppTraits;
}


MainApp::MainApp()
: wxApp()
{
    //wxLog::EnableLogging(false);
    // and now let's initialize our configuration
    config::Install(registrate::GetAppName());
}


// 'Main program' equivalent: the program execution "starts" here
bool MainApp::OnInit()
{
    wxApp::OnInit();
    return true;

    /*
    // create the main application window
    // which in our it's user provided class derived from MainFrame
    if (wxFrame *frame = createMainFrame())
    {
        // and show it (the frames, unlike simple controls, are not shown when
        // created initially)
        frame->Show(true);
        // success: wxApp::OnRun() will be called which will enter the main message
        // loop and the application will run. If we returned false here, the
        // application would exit immediately.
        return true;
    }

    return false;
    */
}


int MainApp::OnExit()
{
    //Message(mtype::kShutdownEnd).Send();
    return wxApp::OnExit();
}


// ----------------------------------------------------------------------------
// event tables and other macros for wxWidgets
// ----------------------------------------------------------------------------

// the event tables connect the wxWidgets events with the functions (event
// handlers) which process them. It can be also done at run-time, but for the
// simple menu events like this the static method is much simpler.
BEGIN_EVENT_TABLE(MainFrame, wxFrame)
    EVT_ERASE_BACKGROUND(MainFrame::OnEraseBackground)
    EVT_IDLE(MainFrame::OnIdle)
    EVT_CLOSE(MainFrame::OnClose)
    EVT_SIZE(MainFrame::OnSize)
    EVT_SIZING(MainFrame::OnSizing)
    EVT_MOUSE_EVENTS(MainFrame::OnMouse)
    EVT_MOUSEWHEEL(MainFrame::OnMouseWheel)
    EVT_KEY_DOWN(MainFrame::OnKeyDown)
    EVT_KEY_UP(MainFrame::OnKeyUp)
    EVT_CHAR(MainFrame::OnChar)
END_EVENT_TABLE()


// ----------------------------------------------------------------------------
// main frame
// ----------------------------------------------------------------------------

// frame constructor
MainFrame::MainFrame(Dispatcher& dispatcher, InputManager& im, ClockManager& clockSource, const std::string& title, const wxPoint& pos, const wxSize& size)
: wxFrame(NULL, wxID_ANY, title.c_str(), pos, size)
, mFrameChanged(false)
, mIdleState(isOff)
, mbProfilerOn(false)
, mProfilerMode(0)
, mbProfilerGraph(false)
, mClockSource(clockSource)
, mInputMananger(im)
{
    // set the frame icon
    SetIcon(wxICON(sample));

    mClock.reset(new Clock(mClockSource));

    dispatcher[mtype::kGetRendererInitData].connect(boost::bind(&MainFrame::OnPluginRequests, this, _1));
    dispatcher[mtype::kAllowTools].connect(boost::bind(&MainFrame::OnPluginRequests, this, _1));
    dispatcher[mtype::kGetValidWindow].connect(boost::bind(&MainFrame::OnPluginRequests, this, _1));
    dispatcher[mtype::kFrameTickRefresh].connect(boost::bind(&MainFrame::onFrameTickRefresh, this, _1));
    dispatcher[mtype::kEndRenderFrame].connect(0, boost::bind(&MainFrame::onProfilerRender, this, _1));

    mAppQuit = mInputMananger.ListenForAction("App.Quit", boost::bind(&MainFrame::InputCallback_Quit, this, _1, _2, _3, _4));
    mProfilerToggle = mInputMananger.ListenForAction("Profiler.Toggle", boost::bind(&MainFrame::onProfilerAction, this, _1, _2, _3, _4));
    mProfilerReportMode = mInputMananger.ListenForAction("Profiler.ReportMode", boost::bind(&MainFrame::onProfilerAction, this, _1, _2, _3, _4));
    mProfilerMoveUp = mInputMananger.ListenForAction("Profiler.Cursor.Move.Up", boost::bind(&MainFrame::onProfilerAction, this, _1, _2, _3, _4));
    mProfilerMoveDown = mInputMananger.ListenForAction("Profiler.Cursor.Move.Down", boost::bind(&MainFrame::onProfilerAction, this, _1, _2, _3, _4));
    mProfilerSelectZone = mInputMananger.ListenForAction("Profiler.Cursor.Select.Zone", boost::bind(&MainFrame::onProfilerAction, this, _1, _2, _3, _4));
    mProfilerSelectParent = mInputMananger.ListenForAction("Profiler.Cursor.Select.Parent", boost::bind(&MainFrame::onProfilerAction, this, _1, _2, _3, _4));
    mProfilerToggleDisplay = mInputMananger.ListenForAction("Profiler.Toggle.Display", boost::bind(&MainFrame::onProfilerAction, this, _1, _2, _3, _4));

    mIdleState = isBegin;
}


void MainFrame::onProfilerRender(Message& /*msg*/)
{
    if (mbProfilerOn)
    {
        wxSize clientSize = GetClientSize();
        if (mbProfilerGraph)
        {
            /*
            *    <sx, sy>      --  origin of the graph--location of (0,0)
            *    x_spacing     --  screenspace size of each history sample; e.g.
            *                         2.0 pixels
            *    y_spacing     --  screenspace size of one millisecond of time;
            *                         for an app with max of 20ms in any one zone,
            *                         8.0 would produce a 160-pixel tall display,
            *                         assuming screenspace is in pixels
            */
            Prof_draw_graph_dx(10, 10, 4, 16, clientSize.x-20, clientSize.y-20);
        }
        else
        {
            /*
            *    <sx,sy>         --  location where top line is drawn
            *    <width, height> --  total size of display (if too small, text will overprint)
            *    line_spacing    --  how much to move sy by after each line; use a
            *                        negative value if y decreases down the screen
            *    precision       --  decimal places of precision for time data, 1..4 (try 2)
            *    print_text      --  function to display a line of text starting at the
            *                        given coordinate; best if 0,1..9 are fixed-width
            */
            Prof_draw_dx(10, 10, clientSize.x-20, clientSize.y-20, 15, 4);
        }
    }
}


void MainFrame::onProfilerAction(const std::string& actionName, uint32_t /*timeStamp*/, int32_t /*mouseX*/, int32_t /*mouseY*/)
{
    if (actionName == "Profiler.Toggle")
    {
        mbProfilerOn = !mbProfilerOn;
        if (mbProfilerOn)
        {
            mInputMananger.PushContext("Profiler");
        }
        else
        {
            std::string lastContext = mInputMananger.PopContext();
            wxASSERT(lastContext == std::string("Profiler"));
        }
    }
    else if (actionName == "Profiler.ReportMode")
    {
        mProfilerMode = ++mProfilerMode % 3;
        wxLogTrace("pong", "CurrentMode: %d", mProfilerMode);
        Prof_set_report_mode((Prof_Report_Mode)mProfilerMode);
    }
    else if (actionName == "Profiler.Cursor.Move.Up")
    {
        Prof_move_cursor(-1);
    }
    else if (actionName == "Profiler.Cursor.Move.Down")
    {
        Prof_move_cursor(1);
    }
    else if (actionName == "Profiler.Cursor.Select.Zone")
    {
        Prof_select();
    }
    else if (actionName == "Profiler.Cursor.Select.Parent")
    {
        Prof_select_parent();
    }
    else if (actionName == "Profiler.Toggle.Display")
    {
        mbProfilerGraph = !mbProfilerGraph;
    }
}


void MainFrame::InputCallback_Quit(const std::string& /*actionName*/, uint32_t /*timeStamp*/, int32_t /*mouseX*/, int32_t /*mouseY*/)
{
    Close();
}


void MainFrame::OnPluginRequests(Message& msg)
{
    switch (msg.mType)
    {
        // return video resolution settings
        case mtype::kGetRendererInitData:
            if (RenderInitData *pRenderInitData = msg.GetValue<RenderInitData *>())
            {
                wxSize winClientSize = GetClientSize();
                //wxPoint winClientSize = ConfigReadP2( _T("Video/Resolution"), wxPoint(640, 480));
                pRenderInitData->mResX = winClientSize.x;
                pRenderInitData->mResY = winClientSize.y;
                pRenderInitData->mWidgetHandle = GetHandle();

                if (config::ReadBool("Video/FreeRefresh"))
                {
                    pRenderInitData->mFlags |= IRenderer::rfFreeRefreshMode;
                }
            }
            break;

        // this will allow or not any running tools
        case mtype::kAllowTools:
            if (true)//(we_are_in_window_mode)
            {
                msg.SetValue(true);
            }
            else
            {
                msg.SetValue(false);
            }

            break;

        case mtype::kGetValidWindow:
            msg.SetValue(static_cast<wxWindow *>(this));
            break;
    }
}


// event handlers
void MainFrame::OnEraseBackground(wxEraseEvent& event)
{
    event.Skip();
}


void MainFrame::OnKeyDown(wxKeyEvent& event)
{
    //wxLogDebug("Key Down: %d", event.GetKeyCode());
    uint32_t flags = InputTypes::kButtonDown;
    flags |= event.AltDown() ? InputTypes::kButtonAlt : 0;
    flags |= event.ControlDown() ? InputTypes::kButtonCtrl : 0;
    flags |= event.ShiftDown() ? InputTypes::kButtonShift : 0;

    int keyValue = mInputMananger.MapKey(event.GetKeyCode());
    InputManager::Key *pKey = new InputManager::Key(getTimeStamp(mClockSource), flags, keyValue);
    mInputMananger.ProcessInput(pKey);

    event.Skip();
}


void MainFrame::OnKeyUp(wxKeyEvent& event)
{
    uint32_t flags = InputTypes::kButtonUp;
    flags |= event.AltDown() ? InputTypes::kButtonAlt : 0;
    flags |= event.ControlDown() ? InputTypes::kButtonCtrl : 0;
    flags |= event.ShiftDown() ? InputTypes::kButtonShift : 0;

    unsigned char keyValue = mInputMananger.MapKey(event.GetKeyCode());
    InputManager::Key *pKey = new InputManager::Key(getTimeStamp(mClockSource), flags, keyValue);
    mInputMananger.ProcessInput(pKey);

    event.Skip();
}


void MainFrame::OnChar(wxKeyEvent& event)
{
    event.Skip();
}


void MainFrame::OnMouseWheel(wxMouseEvent& event)
{
    event.Skip();
}


void MainFrame::OnMouse(wxMouseEvent& event)
{
    uint32_t flags = 0;

    flags |= event.AltDown() ? InputTypes::kButtonAlt : 0;
    flags |= event.ControlDown() ? InputTypes::kButtonCtrl : 0;
    flags |= event.ShiftDown() ? InputTypes::kButtonShift : 0;
    flags |= (event.Moving() || event.Dragging()) ? InputTypes::kMouseMove : 0;

    if (event.ButtonDown())
    {
        flags |= InputTypes::kButtonDown;
    }
    else if (event.ButtonUp())
    {
        flags |= InputTypes::kButtonUp;
    }

    wxPoint mousePos = event.GetPosition();
    InputManager::Mouse *pMouse = new InputManager::Mouse(getTimeStamp(mClockSource), flags, Vector2(mousePos.x, mousePos.y));

    if (event.LeftUp())
    {
        pMouse->mButtons[InputTypes::kMouseLeft] = true;
    }
    else if (!(flags & InputTypes::kButtonUp) && event.LeftIsDown())
    {
        pMouse->mButtons[InputTypes::kMouseLeft] = true;
    }

    if (event.RightUp())
    {
        pMouse->mButtons[InputTypes::kMouseRight] = true;
    }
    else if (!(flags & InputTypes::kButtonUp) && event.RightIsDown())
    {
        pMouse->mButtons[InputTypes::kMouseRight] = true;
    }

    if (event.MiddleUp())
    {
        pMouse->mButtons[InputTypes::kMouseMiddle] = true;
    }
    else if (!(flags & InputTypes::kButtonUp) && event.MiddleIsDown())
    {
        pMouse->mButtons[InputTypes::kMouseMiddle] = true;
    }

    if (flags & InputTypes::kButtonDown)
    {
        CaptureMouse();
    }
    else if (flags & InputTypes::kButtonUp)
    {
        ReleaseMouse();
    }

    mInputMananger.ProcessInput(pMouse);
    mInputMananger.SetCurrentMousePos(Vector2(mousePos.x, mousePos.y));

    event.Skip();
}


// event handlers
void MainFrame::OnSize(wxSizeEvent& event)
{
    wxSize size = GetClientSize();

    mFrameChanged = true;
    event.Skip();
}

void MainFrame::OnSizing(wxSizeEvent& /*event*/)
{
    wxSize size = GetClientSize();
    onSizing(size);
}

void MainFrame::OnClose(wxCloseEvent& event)
{
    if (event.CanVeto())
    {
        mIdleState = isClosing;
        event.Veto();
    }
    else
    {
        Destroy();
    }
}


void MainFrame::OnIdle(wxIdleEvent& event)
{
    Prof(MainLoop);
    if (mIdleState != isOff)
    {
        //TickSystem();
        mClockSource.FrameStep();

        if (mIdleState == isBegin)
        {
            // this is the first time this idle is running
            mIdleState = isOn;
        }

        if (mIdleState == isClosing)
        {
            mIdleState = isOff;
            //mInputMananger.UnregisterAction("Quit App");

            Message(mtype::kShutdownBegin).Send();
            Message(mtype::kFrameTick, 0.0f).Send();
            Message(mtype::kShutdownEnd).Send();
            Message(mtype::kFrameTick, 0.0f).Send();

            // trigger closing of an app and return from here
            Close(true);
            return;
        }

        if (mFrameChanged)
        {
            mFrameChanged = false;
            wxSize clientSize = GetClientSize();
            Size(clientSize);

            // and now let's send kVideoSizeChanged message
            Vector2 winSize(clientSize.x, clientSize.y);
            Message(mtype::kVideoSizeChanged, winSize).Send();
        }

        event.RequestMore();
        float deltaTime = static_cast<float>(mClock->GetFrameDuration());
        /*double currTime =*/ mClock->GetTime();

        {
            Prof(Message_Tick);
            Message(mtype::kFrameTick, deltaTime).Send();
        }
        {
            Prof(Game_Loop);
            Tick(deltaTime);
        }

        //if (!HasFocus())
        //{
        //    //wxTrace(tr_util, "This app lost focused on the Main Window");
        //    wxMilliSleep(1);
        //}
    }

    wxWakeUpIdle();
    Prof_update(1);
}


void MainFrame::onFrameTickRefresh(Message& /*msg*/)
{
    Message(mtype::kFrameTick, 0.0f).Send();
    Tick(0.0f);
}



} // namespace eg

#endif // 0
