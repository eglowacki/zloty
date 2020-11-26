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
#include "Components/SystemsCoordinator.h"


namespace ttt
{
    class BoardSystem;
    class ScoreSystem;
    class RenderSystem;

    using GlobalCoordinator = yaget::comp::Coordinator<GlobalEntity>;
    using EntityCoordinator = yaget::comp::Coordinator<Entity>;
    using GameCoordinatorSet = yaget::comp::CoordinatorSet<GlobalCoordinator, EntityCoordinator>;

    using GameSystemsCoordinator = yaget::comp::gs::SystemsCoordinator<GameCoordinatorSet, Messaging, BoardSystem, ScoreSystem>;

    using RenderCoordinator = yaget::comp::Coordinator<RenderEntity>;
    using RenderCoordinatorSet = yaget::comp::CoordinatorSet<RenderCoordinator>;

    using RenderSystemsCoordinator = yaget::comp::gs::SystemsCoordinator<RenderCoordinatorSet, Messaging, RenderSystem>;

}

