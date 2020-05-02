//////////////////////////////////////////////////////////////////////
// MainConsole.h
//
//  Copyright 3/15/2007 Edgar Glowacki.
//
//  Maintained by: Edgar
//
//  NOTES:
//      Provides main entry point for console application
//
//
//  #include "App/MainConsole.h"
//
//////////////////////////////////////////////////////////////////////
//! \file

#ifndef MAIN_CONSOLE_H
#define MAIN_CONSOLE_H
#pragma once

#include "MessageInterface.h"
#include <string>
#include <boost/scoped_ptr.hpp>

namespace eg
{

    class MainConsole
    {
    public:
        enum Color
        {
            Black, Blue, Green, Cayn, Red, Pink, Yellow, Gray,
            DGray, LBlue, LGreen, LCayn, LRed, LPink, LYellow, White
        };

        static void ConPrint(const std::string& charBuffer) {ConPrint(charBuffer.c_str(), charBuffer.size());}
        static void ConPrint(const char *pCharBuffer, int len);
        static void ConPrintAt(int x, int y, const std::string& charBuffer) {ConPrintAt(x, y, charBuffer.c_str(), charBuffer.size());}
        static void ConPrintAt(int x, int y, const char *pCharBuffer, int len);
        static void gotoXY(int x, int y);
        static void ClearConsole();
        static void ClearConsole(int ForgC, int BackC);
        static void SetColor(int ForgC, int BackC);
        static void SetColor(int ForgC);
        static void HideCursor();
        static void ShowCursor();
    };

#if 0
    class Clock;

    /*!
    This class needs to be derived by user
    */
    class MainConsole
    {
    public:
        MainConsole(const std::string& title, const wxPoint& pos, const wxSize& size);
        virtual ~MainConsole();

        int Run();

        static void ConPrint(const std::string& charBuffer) {ConPrint(charBuffer.c_str(), charBuffer.size());}
        static void ConPrint(const char *pCharBuffer, int len);
        static void ConPrintAt(int x, int y, const std::string& charBuffer) {ConPrintAt(x, y, charBuffer.c_str(), charBuffer.size());}
        static void ConPrintAt(int x, int y, const char *pCharBuffer, int len);
        static void gotoXY(int x, int y);
        static void ClearConsole();
        static void ClearConsole(int ForgC, int BackC);
        static void SetColor(int ForgC, int BackC);
        static void SetColor(int ForgC);
        static void HideCursor();
        static void ShowCursor();

    private:
        //! Provide this in derived class
        virtual void Tick(float deltaTime) = 0;

        void InputCallback_Quit(const std::string& actionName, uint32_t timeStamp, int32_t mouseX, int32_t mouseY);
        bool mQuit;
        boost::scoped_ptr<Clock> mClock;
        wxAppConsole *mAppConsole;
    };

    //! Helper function to start console
    //! \code
    //!     int result = StartConsole<ServerConsole>(argc, argv);
    //! \endcode
    template <typename T>
    inline int StartConsole(int argc, char* argv[])
    {
        int result = 0;
        wxLog::EnableLogging(false);

        wxInitializer initializeConsole(argc, argv);
        {
            // do this in a scoped block to assure correct dtor order
            T consoleApp;
            result = consoleApp.Run();
        }

        // send one last tick message to give a chance to finish any processing left
        assert(0); //"Fix me"
        //Message(mtype::kFrameTick, 0.0f).Send();
        //Message(mtype::kShutdownEnd).Send();
        return result;
    }

    /*!
    This is useful when we use UnitTest and want to make sure that all internal
    support modules are initialized and shutdown correctly.
    */
    class StartUnitTestSystem
    {
    public:
        StartUnitTestSystem(int argc, char* argv[], bool bEnableLogging = false)
        {
            wxLog::EnableLogging(bEnableLogging);
            wxInitialize(argc, argv);
        }

        void Begin()
        {
        }

        void End()
        {
            assert(0); //"Fix me"
            //Message(mtype::kShutdownBegin).Send();
            //Message(mtype::kFrameTick, 0.0f).Send();
        }

        ~StartUnitTestSystem()
        {
            assert(0); //"Fix me"
            //Message(mtype::kFrameTick, 0.0f).Send();
            //Message(mtype::kShutdownEnd).Send();
            wxUninitialize();
        }
    };

#endif // 0
} // namespace eg

#endif // MAIN_CONSOLE_H

