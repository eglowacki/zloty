///////////////////////////////////////////////////////////////////////
// FrameStateGatherSystem.h
//
//  Copyright 01/12/2026 Edgar Glowacki.
//
//  Maintained by: Edgar
//
//  NOTES:
//
//
//  #include "Renders/FrameStateGatherSystem.h"
//
//////////////////////////////////////////////////////////////////////
//! \file
#pragma once


#include "Components/GameSystem.h"
#include "Render/DesktopApplication.h"
#include "DefensorGameTypes.h"

namespace defensor::render
{
    // thisd should be first system in Coordinator setup
    class FrameStateGatherSystem : public yaget::render::RenderSystemApp<RenderCoordinatorSet, comp::gs::GenerateEndMarker, Messaging, SceneComponent*>
    {
    public:
        FrameStateGatherSystem(Messaging& messaging, Application& app, RenderCoordinatorSet& coordinatorSet);

    private:
        void OnUpdate(comp::Id_t id, const time::GameClock& gameClock, metrics::Channel& channel, SceneComponent* sceneComponent);
    };


    // thisd should be last system in Coordinator setup
    class FrameStateClearSystem : public comp::gs::GameSystem<RenderCoordinatorSet, comp::gs::GenerateEndMarker, defensor::game::Messaging, SceneComponent*>
    {
    public:
        FrameStateClearSystem(Messaging& messaging, Application& app, RenderCoordinatorSet& coordinatorSet);

    private:
        void OnUpdate(comp::Id_t id, const time::GameClock& gameClock, metrics::Channel& channel, SceneComponent* sceneComponent);
    };

}
