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

#include "YagetCore.h"
#include "Components/SystemsCoordinator.h"
#include "GameSystem/Messaging.h"
#include "Components/LocationComponent.h"

namespace defensor::game
{
    using namespace yaget;

    using Messaging = comp::gs::Messaging<std::shared_ptr<char>>;

    //struct EditorComponent { static constexpr int Capacity = 64; };
    //struct EmptyComponent { static constexpr int Capacity = 64; };
    //struct BlankComponent { static constexpr int Capacity = 64; };
    //struct RenderComponent { static constexpr int Capacity = 64; };

    //using RenderEntity = comp::GlobalRowPolicy<RenderComponent*>;

    //using GlobalEntity = comp::GlobalRowPolicy<EditorComponent*>;
    using Entity = comp::RowPolicy<comp::LocationComponent3*>;

    //using GlobalCoordinator = comp::Coordinator<GlobalEntity>;
    using EntityCoordinator = comp::Coordinator<Entity>;
    using GameCoordinatorSet = comp::CoordinatorSet<EntityCoordinator>;

    //using RenderCoordinator = comp::Coordinator<RenderEntity>;
    //using RenderCoordinatorSet = comp::CoordinatorSet<RenderCoordinator>;
}
