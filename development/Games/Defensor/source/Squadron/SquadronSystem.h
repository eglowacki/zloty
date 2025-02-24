///////////////////////////////////////////////////////////////////////
// SquadronSystem.h
//
//  Copyright 6/30/2024 Edgar Glowacki.
//
//  Maintained by: Edgar
//
//  NOTES:
//      System which iterates and tick over entities
//
//
//  #include "Squadron/SquadronSystem.h"
//
//////////////////////////////////////////////////////////////////////
//! \file
#pragma once


#include "Components/GameSystem.h"
#include "DefensorGameTypes.h"

namespace defensor::game
{
    class SquadronSystem : public comp::gs::GameSystem<GameCoordinatorSet, comp::gs::NoEndMarker, defensor::game::Messaging, comp::LocationComponent3*, comp::UnitComponent*>
    {
    public:
        SquadronSystem(Messaging& messaging, Application& app, GameCoordinatorSet& coordinatorSet);

    private:
        void OnUpdate(comp::Id_t id, const time::GameClock& gameClock, metrics::Channel& channel, comp::LocationComponent3* locationComponent, comp::UnitComponent* unitComponent);
    };
}
