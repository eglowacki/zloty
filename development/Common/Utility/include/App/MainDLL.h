//////////////////////////////////////////////////////////////////////
// MainDLL.h
//
//  Copyright 11/21/2007 Edgar Glowacki.
//
//  Maintained by: Edgar
//
//  NOTES: Utility functions to easy of startup, shutdown and game loop
//         managements
//
//
//  #include "App/MainDLL.h"
//
//////////////////////////////////////////////////////////////////////
//! \file

#ifndef _APP_MAINDLL_H
#define _APP_MAINDLL_H
#pragma once

#if 0
#include "MessageInterface.h"
#include "Timer/Clock.h"
#include "Registrate.h"
#include "App/AppTraits.h"
#include <wx/wx.h>
#include <boost/scoped_ptr.hpp>



namespace eg
{
    namespace internal
    {
        class DllApp : public wxAppConsole
        {
        public:
            DllApp() {}
            virtual int OnRun() {wxFAIL_MSG( _T("unreachable code") ); return 0;}

        private:
            virtual wxAppTraits *CreateTraits()
            {
                return new AppTraits;
            }
        };

        wxAppConsole *DllAppInitializerFunction()
        {
            return new DllApp;
        }

    } // namespace internal

    class MainDll
    {
    public:
        MainDll()
        : mInitialized(false)
        {
            wxApp::SetInitializerFunction(internal::DllAppInitializerFunction);
            mInitialized = wxInitialize(0, 0);

            wxASSERT(mInitialized);
            if (mInitialized)
            {
                ClockManager& cm = REGISTRATE(ClockManager);
                mClock.reset(new Clock(cm));
            }
        }

        virtual ~MainDll()
        {
            if (mInitialized)
            {
                assert(0, "Fix me");
                //Message(mtype::kShutdownBegin).Send();
                //Message(mtype::kFrameTick, 0.0f).Send();
                wxUninitialize();
            }
        }

        void Tick()
        {
            if (mInitialized)
            {
                assert(0, "Fix me");
                //TickSystem();

                float deltaTime = static_cast<float>(mClock->GetFrameDuration());
                double currTime = mClock->GetTime();
                //Message(mtype::kFrameTick, deltaTime).Send();

                onTick(deltaTime);
            }
        }

    private:
        virtual void onTick(float deltaTime) = 0;

        boost::scoped_ptr<Clock> mClock;

        bool mInitialized;
    };

} // namespace eg

#endif // 0
#endif // _APP_MAINDLL_H

