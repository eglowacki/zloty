///////////////////////////////////////////////////////////////////////
// MenuSystem.h
//
//  Copyright 8/04/2024 Edgar Glowacki.
//
//  Maintained by: Edgar
//
//  NOTES:
//
//
//  #include "Squadron/MenuSystem.h"
//
//////////////////////////////////////////////////////////////////////
//! \file
#pragma once


#include "Components/GameSystem.h"
#include "DefensorGameTypes.h"

namespace defensor::game
{
    class MenuSystem : public comp::gs::GameSystem<GameCoordinatorSet, comp::gs::NoEndMarker, defensor::game::Messaging, comp::MenuComponent*, comp::InputComponent*, comp::ScriptComponent*>
    {
    public:
        MenuSystem(Messaging& messaging, Application& app, GameCoordinatorSet& coordinatorSet);

    private:
        void OnUpdate(comp::Id_t id, const time::GameClock& gameClock, metrics::Channel& channel, comp::MenuComponent* menuComponent, comp::InputComponent* inputComponent, comp::ScriptComponent* scriptComponent);
    };

}
