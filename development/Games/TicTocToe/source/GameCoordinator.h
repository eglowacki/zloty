///////////////////////////////////////////////////////////////////////
// GameCoordinator.h
//
//  Copyright 10/10/2020 Edgar Glowacki.
//
//  Maintained by: Edgar
//
//  NOTES:
//      
//
//
//  #include "GameCoordinator.h"
//
//////////////////////////////////////////////////////////////////////
//! \file
#pragma once

#include "GameTypes.h"
#include "Components/GameSystemsCoordinator.h"


namespace ttt
{
    class BoardSystem;
    class ScoreSystem;

    //using GameCoordinator = yaget::GameCoordinator<GamePolicy>;

    using GlobalCoordinator = yaget::comp::Coordinator<GlobalEntity>;
    using EntityCoordinator = yaget::comp::Coordinator<Entity>;
    using GameCoordinatorSet = yaget::comp::CoordinatorSet<GlobalCoordinator, EntityCoordinator>;

    using GameSystemsCoordinator = yaget::comp::gs::GameSystemsCoordinator<GameCoordinatorSet, BoardSystem, ScoreSystem>;

}

