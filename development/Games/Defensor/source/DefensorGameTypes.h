///////////////////////////////////////////////////////////////////////
// DefensorGameTypes.h
//
//  Copyright 06/26/2024 Edgar Glowacki.
//
//  Maintained by: Edgar
//
//  NOTES:
//      Specific game types needed for Defensor game
//
//
//  #include "DefensorGameTypes.h"
//
//////////////////////////////////////////////////////////////////////
//! \file
#pragma once

#include "Components/InputComponent.h"
#include "Components/LocationComponent.h"
#include "Components/MenuComponent.h"
#include "Components/ScriptComponent.h"
#include "Components/SystemsCoordinator.h"
#include "Components/UnitComponent.h"
#include "GameSystem/Messaging.h"
#include "Items/StageComponent.h"

namespace defensor::game
{
    using namespace yaget;

    using Messaging = comp::gs::Messaging<std::shared_ptr<char>>;

    using GlobalEntity = comp::GlobalRowPolicy<comp::MenuComponent*, items::StageComponent*>;
    using Entity = comp::RowPolicy<comp::LocationComponent3*, comp::InputComponent*, comp::UnitComponent*, comp::ScriptComponent*>;

    using GlobalCoordinator = comp::Coordinator<GlobalEntity>;
    using EntityCoordinator = comp::Coordinator<Entity>;
    using GameCoordinatorSet = comp::CoordinatorSet<GlobalCoordinator, EntityCoordinator>;

}
