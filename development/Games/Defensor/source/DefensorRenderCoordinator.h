///////////////////////////////////////////////////////////////////////
// DefensorRenderCoordinator.h
//
//  Copyright 01/11/2026 Edgar Glowacki.
//
//  Maintained by: Edgar
//
//  NOTES:
//      
//
//
//  #include "DefensorRenderCoordinator.h"
//
//////////////////////////////////////////////////////////////////////
//! \file
#pragma once

//#include "YagetCore.h"
#include "Components/SystemsCoordinator.h"
#include "DefensorGameTypes.h"
#include "Renders/RenderSystem.h"
#include "Renders/FrameStateGatherSystem.h"


namespace defensor::render
{
    class DefensorSystemsCoordinator : public yaget::comp::gs::SystemsCoordinator<RenderCoordinatorSet, Messaging, Application, FrameStateGatherSystem, RenderSystem, FrameStateClearSystem>
    {
     public:
        DefensorSystemsCoordinator(Messaging& m, yaget::Application& app);
        ~DefensorSystemsCoordinator();

        void Tick(const time::GameClock& gameClock, metrics::Channel& channel);

    private:
        yaget::render::DesktopApplication& mApplication;
    };

}
