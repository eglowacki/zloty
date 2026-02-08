///////////////////////////////////////////////////////////////////////
// FrameStateCollectorSystem.h
//
//  Copyright 01/11/2026 Edgar Glowacki.
//
//  Maintained by: Edgar
//
//  NOTES:
//      This system collects data from logic/game thread and packages into 
//      MessagingPayload structure, which then gets used by Render thread
//      to actually render the geometry representing that particular
//      asset (item/entity). There are structures like render::EntityState
//      that provide interface for marshaling data between threads.
//
//      render::FrameStateGatherSystem
//          This is a render thread, and it collects data from MessagingPayload and prepares it for rendering. It also
//          gathers some additional data like camera position, lighting, etc. and prepares render::FrameState structure, 
//          which is then used by render::RenderSystem to actually render the frame.
//
//      render::RenderSystem
//          This is a render thread, and it takes render::FrameState structure prepared by render::FrameStateGatherSystem 
//          and uses it to render the frame. It also handles some additional tasks like post-processing, etc.
//
//      render::FrameStateClearSystem
//          This is a render thread, and it clears the data from previous frame after rendering is done. 
//          It also handles some additional tasks like resetting the state, etc.
//
//
//  #include "Systems/FrameStateCollectorSystem.h"
//
//////////////////////////////////////////////////////////////////////
//! \file
#pragma once

#include "DefensorGameTypes.h"
#include "Components/GameSystem.h"

namespace defensor::game
{
    class FrameStateCollectorSystem : public comp::gs::GameSystem<GameCoordinatorSet, comp::gs::GenerateEndMarker, defensor::game::Messaging, comp::LocationComponent3*, comp::MaterialComponent*>
    {
    public:
        FrameStateCollectorSystem(Messaging& messaging, Application& app, GameCoordinatorSet& coordinatorSet);

    private:
        void OnUpdate(comp::Id_t id, const time::GameClock& gameClock, metrics::Channel& channel, comp::LocationComponent3* locationComponent, comp::MaterialComponent* materialComponent);

        MessagingPayload mCurrentFrameState;
    };

}
