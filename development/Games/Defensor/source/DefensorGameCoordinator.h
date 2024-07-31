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
#include "Squadron/SquadronSystem.h"
#include <Components/SystemsCoordinator.h>


namespace defensor::game
{
    class DefensorSystemsCoordinator : public yaget::comp::gs::SystemsCoordinator<GameCoordinatorSet, Messaging, Application, SquadronSystem>
    {
    public:
        DefensorSystemsCoordinator(Messaging& m, Application& app);
        ~DefensorSystemsCoordinator() = default;
    };
}
