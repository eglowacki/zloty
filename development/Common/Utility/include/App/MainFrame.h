///////////////////////////////////////////////////////////////////////
// MainFrame.h
//
//  Copyright 11/30/2006 Edgar Glowacki.
//
//  Maintained by: Edgar
//
//  NOTES:
//      Frame object which represents main window in the app
//
//
//  #include "App/MainFrame.h"
//
//////////////////////////////////////////////////////////////////////
//! \file

#ifndef APP_MAIN_FRAME_H
#define APP_MAIN_FRAME_H
#pragma once
#if 0
#include "Base.h"
#include "Input/InputManager.h"
#include <wx/wx.h>
#include <boost/shared_ptr.hpp>


namespace eg
{
    class Clock;
    class Message;
    class MainFrame;
    class ClockManager;

    /*!
    This App object is very light wrapper around wxApp
    We do not do any work there and all user interaction
    is done with user class derived from MainFrame
    */
    class MainApp : public wxApp
    {
    public:
        MainApp();
        // override base class virtual
        // ----------------------------

        // this one is called on application startup and is a good place for the app
        // initialization (doing it here and not in the ctor allows to have an error
        // return: if OnInit() returns false, the application terminates)
        virtual bool OnInit();
        virtual int OnExit();

    private:
        //! This needs to return main frame window
        //virtual wxFrame *createMainFrame() = 0;
        virtual wxAppTraits *CreateTraits();
    };

    //DECLARE_APP(MainApp);


    class MainFrame : public wxFrame
    {
    public:
        // ctor(s)
        MainFrame(Dispatcher& dispatcher, InputManager& im, ClockManager& clockSource, const std::string& title, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize);

        // event handlers (these functions should _not_ be virtual)
        //void OnQuit(wxCommandEvent& event);
        void OnEraseBackground(wxEraseEvent& event);
        void OnIdle(wxIdleEvent& event);
        void OnClose(wxCloseEvent& event);
        void OnSize(wxSizeEvent& event);
        void OnSizing(wxSizeEvent& event);
        void OnMouse(wxMouseEvent& event);
        void OnMouseWheel(wxMouseEvent& event);
        void OnKeyDown(wxKeyEvent& event);
        void OnKeyUp(wxKeyEvent& event);
        void OnChar(wxKeyEvent& event);

    private:
        //! Provide this in your derive class and do entire game loop there including flip buffer
        virtual void Tick(float deltaTime) = 0;
        virtual void Size(const wxSize& size) = 0;
        virtual void onSizing(const wxSize& /*size*/) {}

        void InputCallback_Quit(const std::string& actionName, uint32_t timeStamp, int32_t mouseX, int32_t mouseY);

        //! Used by any plugin request
        void OnPluginRequests(Message& msg);
        void onFrameTickRefresh(Message& msg);

        //! current idle state, where
        //! isOff - not running
        //! isBegin - set in ctor, and in Idle method, it is used to set first time
        //! isOn - running on each idle event
        //! isClosing - send shutdown messages and request app quit and set state to isOff
        enum eIdleState
        {
            isOff, isBegin, isOn, isClosing
        };
        eIdleState mIdleState;

        //! Used in OnSize handler, so on left mouse up, it will reset internal state
        bool mFrameChanged;
        //! our main clock
        boost::shared_ptr<Clock> mClock;

        //! controls if Profiler is on
        bool mbProfilerOn;
        //! controls display of profiler (text, graph)
        bool mbProfilerGraph;
        //! mode of profile display (only relevant to text mode)
        int mProfilerMode;

        void onProfilerRender(Message& msg);
        void onProfilerAction(const std::string& actionName, uint32_t timeStamp, int32_t mouseX, int32_t mouseY);
        eg::InputManager::ActionListener_t mProfilerToggle;
        eg::InputManager::ActionListener_t mProfilerReportMode;
        eg::InputManager::ActionListener_t mProfilerMoveUp;
        eg::InputManager::ActionListener_t mProfilerMoveDown;
        eg::InputManager::ActionListener_t mProfilerSelectZone;
        eg::InputManager::ActionListener_t mProfilerSelectParent;
        eg::InputManager::ActionListener_t mProfilerToggleDisplay;
        eg::InputManager::ActionListener_t mAppQuit;

        //! real time source
        ClockManager& mClockSource;
        InputManager& mInputMananger;

        // any class wishing to process wxWidgets events must use this macro
        DECLARE_EVENT_TABLE()
    };

    //! This needs to be supplied by user app and it should return new object derived from MainFrame
    //! MainAPp will take an ownership of this pointer and it will delete it on exit
    //wxFrame *GetMainUserFrame();

} // namespace eg

#endif // 0

#endif // APP_MAIN_FRAME_H
