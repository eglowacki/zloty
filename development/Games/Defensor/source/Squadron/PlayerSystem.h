///////////////////////////////////////////////////////////////////////
// PlayerSystem.h
//
//  Copyright 8/02/2024 Edgar Glowacki.
//
//  Maintained by: Edgar
//
//  NOTES:
//
//
//  #include "Squadron/PlayerSystem.h"
//
//////////////////////////////////////////////////////////////////////
//! \file
#pragma once


#include "Components/GameSystem.h"
#include "DefensorGameTypes.h"

namespace defensor::game
{
    class PlayerSystem : public comp::gs::GameSystem<GameCoordinatorSet, comp::gs::NoEndMarker, defensor::game::Messaging, comp::LocationComponent3*, comp::InputComponent*>
    {
    public:
        PlayerSystem(Messaging& messaging, Application& app, GameCoordinatorSet& coordinatorSet);

    private:
        using Section = io::VirtualTransportSystem::Section;

        void OnUpdate(comp::Id_t id, const time::GameClock& gameClock, metrics::Channel& channel, comp::LocationComponent3* locationComponent, const comp::InputComponent* inputComponent);
        void AddPlayerBullet(comp::Id_t id, const std::string& itemName, const Section& materialSection, math3d::Vector3 bulletPosition);

        bool mBulletActive = false;
    };
}
