///////////////////////////////////////////////////////////////////////
// DefensorGameCoordinator.h
//
//  Copyright 06/30/2024 Edgar Glowacki.
//
//  Maintained by: Edgar
//
//  NOTES:
//      
//
//
//  #include "DefensorGameCoordinator.h"
//
//////////////////////////////////////////////////////////////////////
//! \file
#pragma once

//#include "YagetCore.h"
#include "Components/SystemsCoordinator.h"
#include "Squadron/MenuSystem.h"
#include "Squadron/PlayerSystem.h"
#include "Squadron/SquadronSystem.h"
#include "Stager/StagerSystem.h"
#include "Systems/InputSystem.h"
#include "Systems/FrameStateCollectorSystem.h"


namespace defensor::game
{
    class DefensorSystemsCoordinator : public yaget::comp::gs::SystemsCoordinator<GameCoordinatorSet, Messaging, Application, ProcessInputSystem, MenuSystem, PlayerSystem, SquadronSystem, DefensorStagerSystem, ClearInputSystem, FrameStateCollectorSystem>
    {
    public:
        DefensorSystemsCoordinator(Messaging& m, Application& app);
        ~DefensorSystemsCoordinator() = default;

        // only used to show off that we can override any method from base class without virtual inheritance
        void Tick(const time::GameClock& gameClock, metrics::Channel& channel);

    };
}
