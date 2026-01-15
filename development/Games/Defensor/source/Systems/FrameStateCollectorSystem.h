///////////////////////////////////////////////////////////////////////
// FrameStateCollectorSystem.h
//
//  Copyright 01/11/2026 Edgar Glowacki.
//
//  Maintained by: Edgar
//
//  NOTES:
//
//
//  #include "Systems/FrameStateCollectorSystem.h"
//
//////////////////////////////////////////////////////////////////////
//! \file
#pragma once


#include "Components/GameSystem.h"
#include "DefensorGameTypes.h"

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
