///////////////////////////////////////////////////////////////////////
// FrameStateCollectorSystem.h
//
//  Copyright 01/11/2026 Edgar Glowacki.
//
//  Maintained by: Edgar
//
//  NOTES:
//      This system collects data from logic/game thread and packages into 
//      MessagingPayload structure, which then get's used by Reder thread
//      to actually render the geometry representing that particular
//      asset (item/entity). There are stuctures like render::EntityState
//      that provide interface for marshaling data between threads.
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
    class FrameStateCollectorSystem : public comp::gs::GameSystem<GameCoordinatorSet, comp::gs::GenerateEndMarker, defensor::game::Messaging, comp::LocationComponent3*>
    {
    public:
        FrameStateCollectorSystem(Messaging& messaging, Application& app, GameCoordinatorSet& coordinatorSet);

    private:
        void OnUpdate(comp::Id_t id, const time::GameClock& gameClock, metrics::Channel& channel, comp::LocationComponent3* locationComponent);

        MessagingPayload mCurrentFrameState;
    };

}
