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

    class ConsoleApplication : public Application
    {
    public:
        ConsoleApplication(const std::string& title, items::Director& director, io::VirtualTransportSystem& vts, const args::Options& options);
        virtual ~ConsoleApplication();
        math3d::Vector2 GetWindowSize() const override;

#if  0
        void Print(const std::string& charBuffer) override;
        void PrintAt(int x, int y, const std::string& charBuffer) override;
        void GotoXY(int x, int y) override;
        void Clear(int ForgC = -1, int BackC = -1) override;
        void SetColor(int ForgC, int BackC = -1) override;
        void SetColor(float r, float g, float b, float a) override;
        void Cursor(bool bShow) override;
        void SetCursorLoc(int x, int y) override;
#endif // 0

    private:
        bool onMessagePump(const time::GameClock& gameClock) override;
        void Cleanup() override;

        WindowHandle_t mOutputHandle = nullptr;
    };

    namespace app
    {
        class DefaultConsole : public ConsoleApplication
        {
        public:
            DefaultConsole(const std::string& title, items::Director& director, io::VirtualTransportSystem& vts, const args::Options& options);
        };

        class BlankApplication : public Application
        {
        public:
            BlankApplication(const std::string& title, items::Director& director, io::VirtualTransportSystem& vts, const args::Options& options);

            math3d::Vector2 GetWindowSize() const override;

        private:
            bool onMessagePump(const time::GameClock& gameClock) override;
            void Cleanup() override;
        };

    } // namespace app

} // namespace yaget

