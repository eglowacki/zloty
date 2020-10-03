//////////////////////////////////////////////////////////////////////
// ConsoleApplication.h
//
//  Copyright 7/22/2016 Edgar Glowacki.
//
//  Maintained by: Edgar
//
//  NOTES:
//      Provides main entry point for console application
//
//
//  #include "App/ConsoleApplication.h"
//
//////////////////////////////////////////////////////////////////////
//! \file

#pragma once

#include "Application.h"
#include <string>

namespace yaget
{
    namespace io { class VirtualTransportSystem; }

    //! Used in console applications
    class ConsoleApplication : public Application
    {
    public:
        ConsoleApplication(const std::string& title, items::Director& director, io::VirtualTransportSystem& vts, const args::Options& options);
        virtual ~ConsoleApplication();

        app::DisplaySurface GetSurface() const override { return app::DisplaySurface{ mOutputHandle, app::SurfaceState::Shared }; };

    private:
        bool onMessagePump(const time::GameClock& gameClock) override;
        void Cleanup() override;

        WindowHandle_t mOutputHandle = nullptr;
    };

    namespace app
    {
        // this adds quit (ESC) to input action, otherwise it's the same as ConsoleApplication
        class DefaultConsole : public ConsoleApplication
        {
        public:
            DefaultConsole(const std::string& title, items::Director& director, io::VirtualTransportSystem& vts, const args::Options& options);
        };

        class BlankApplication : public Application
        {
        public:
            BlankApplication(const std::string& title, items::Director& director, io::VirtualTransportSystem& vts, const args::Options& options);

            const yaget::time::GameClock& GameClock() const { return mGameClock; }

            app::DisplaySurface GetSurface() const override { return app::DisplaySurface{ nullptr, app::SurfaceState::Shared }; };

        private:
            bool onMessagePump(const time::GameClock& gameClock) override;
            void Cleanup() override;
        };

    } // namespace app

} // namespace yaget

