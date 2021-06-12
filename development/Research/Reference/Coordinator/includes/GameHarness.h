//////////////////////////////////////////////////////////////////////
// GameHarness.h
//
//  Copyright 7/4/2019 Edgar Glowacki
//
//  Maintained by: Edgar
//
//  NOTES:
//      Attempt at managing logic, render thread and coordinators
//
//
//  #include "GameHarness.h"
//
//////////////////////////////////////////////////////////////////////
//! \file
#pragma once

#include "YagetCore.h"
#include "Components/GameCoordinator.h"
#include "App/Application.h"
#include <functional>



namespace yaget
{
    namespace time { class GameClock; }
    namespace metrics { class Channel; }
    namespace render { class Device; }
    class Application;
    class IGameCoordinator;

    // This class is used as a bootstrap and then handling both game and render loop.
    // Main job is to call IGameCoordinator Update and Render method with correct parameters
    // including managing device render frame. It also provides virtual overloads for those
    // 4 basic types of ticks and are called as a first thing in respective methods.
    class GameHarness : public Noncopyable<GameHarness>
    {
    public:
        GameHarness(IGameCoordinator& gameCoordinator, Application& app, render::Device& device);
        virtual ~GameHarness() = default;

        int Run();

    protected:
        IGameCoordinator& mGameCoordinator;

    private:
        void RenderLoopTick(Application& app, const time::GameClock& gameClock, metrics::Channel& channel);
        void IdleTick();
        void QuitTick();

        // handles state change from created to initialized so we can call actual initialization on objects
        void OnInitialize(Application& app, const time::GameClock& gameClock, metrics::Channel& channel);
        void OnUpdate(Application& app, const time::GameClock& gameClock, metrics::Channel& channel);
        void OnShutdown(Application& app, const time::GameClock& gameClock, metrics::Channel& channel);

        virtual void onGameLoop(Application& /*app*/, const time::GameClock& /*gameClock*/, metrics::Channel& /*channel*/) {}
        virtual void onRenderLoop(Application& /*app*/, const time::GameClock& /*gameClock*/, metrics::Channel& /*channel*/) {}
        virtual void onIdle() {}
        virtual void onQuit() {}

        Application& mApplication;
        render::Device& mDevice;

        using UpdateFrame = std::function<void(Application& /*app*/, const time::GameClock& /*gameClock*/, metrics::Channel& /*channel*/)>;
        UpdateFrame mUpdateFrame;
    };

    namespace exploration
    {
        class GameHarness : public Noncopyable<GameHarness>
        {
        public:
            GameHarness(IGameCoordinator& gameCoordinator, Application& app, render::Device& device)
                : mGameCoordinator(gameCoordinator)
                , mApplication(app)
                , mDevice(device)
                , mUpdateFrame([this](auto&&... params) { OnInitialize(params...); })
            {
            }
            virtual ~GameHarness() = default;

            int Run()
            {
                auto gameLoop = [this](auto&&... params) { mUpdateFrame(params...); };
                auto renderLoop = [this](auto&&... params) { RenderLoopTick(params...); };
                auto idleLoop = [this] { IdleTick(); };
                auto quitLoop = [this] { QuitTick(); };
                auto shutdownLogicLoop = [this](auto&&... params) { OnShutdown(params...); };

                return mApplication.Run(gameLoop, renderLoop, idleLoop, shutdownLogicLoop, quitLoop);
            }

        protected:
            IGameCoordinator& mGameCoordinator;

        private:
            void RenderLoopTick(Application& /*app*/, const time::GameClock& /*gameClock*/, metrics::Channel& /*channel*/)
            {
                //onRenderLoop(app, gameClock, channel);

                //if (render::RenderTarget* renderTarget = mDevice.FindRenderTarget("BackBuffer"))
                //{
                //    if (dev::CurrentConfiguration().mDebug.mFlags.Gui)
                //    {
                //        renderTarget->AddProcessor(render::RenderTarget::ProcessorType::PostFrame, RenderDemoPanel);
                //        renderTarget->AddProcessor(render::RenderTarget::ProcessorType::PostFrame, RenderLogPanel);
                //    }
                //}

                //// we are ready to process stager from our logic/systems
                //mDevice.RenderFrame(gameClock, channel, nullptr, [this](const time::GameClock& gameClock, metrics::Channel& channel)
                //{
                //    mGameCoordinator.RenderUpdate(gameClock, channel);
                //});
            }

            void IdleTick() {}
            void QuitTick() {}

            // handles state change from created to initialized so we can call actual initialization on objects
            void OnInitialize(Application& /*app*/, const time::GameClock& gameClock, metrics::Channel& channel)
            {
                mGameCoordinator.GameInitialize(gameClock, channel);
                mUpdateFrame = [this](auto&&... params) { OnUpdate(params...); };
            }

            void OnUpdate(Application& /*app*/, const time::GameClock& gameClock, metrics::Channel& channel)
            {
                mGameCoordinator.GameUpdate(gameClock, channel);
                //onGameLoop(app, gameClock, channel);
            }

            void OnShutdown(Application& /*app*/, const time::GameClock& gameClock, metrics::Channel& channel)
            {
                mGameCoordinator.GameShutdown(gameClock, channel);
            }

            //virtual void onGameLoop(Application& /*app*/, const time::GameClock& /*gameClock*/, metrics::Channel& /*channel*/) {}
            //virtual void onRenderLoop(Application& /*app*/, const time::GameClock& /*gameClock*/, metrics::Channel& /*channel*/) {}
            //virtual void onIdle() {}
            //virtual void onQuit() {}

            Application& mApplication;
            render::Device& mDevice;

            using UpdateFrame = std::function<void(Application& /*app*/, const time::GameClock& /*gameClock*/, metrics::Channel& /*channel*/)>;
            UpdateFrame mUpdateFrame;
        };
    }

} // namespace yaget
