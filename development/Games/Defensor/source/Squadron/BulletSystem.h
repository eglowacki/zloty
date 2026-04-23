///////////////////////////////////////////////////////////////////////
// BulletSystem.h
//
//  Copyright 8/04/2024 Edgar Glowacki.
//
//  Maintained by: Edgar
//
//  NOTES:
//
//
//  #include "Squadron/BulletSystem.h"
//
//////////////////////////////////////////////////////////////////////
//! \file
#pragma once


#include "Components/GameSystem.h"
#include "DefensorGameTypes.h"

namespace defensor::game
{
    class BulletSystem : public comp::gs::GameSystem<GameCoordinatorSet, comp::gs::GenerateEndMarker, defensor::game::Messaging, comp::LocationComponent3*, comp::MaterialComponent*, comp::BulletComponent*>
    {
    public:
        BulletSystem(Messaging& messaging, Application& app, GameCoordinatorSet& coordinatorSet);

    private:
        void OnUpdate(comp::Id_t id, const time::GameClock& gameClock, metrics::Channel& channel, comp::LocationComponent3* locationComponent, comp::MaterialComponent* materialComponent, comp::BulletComponent* bulletComponent);

        comp::ItemIds mBulletsToRemove;
    };

}
