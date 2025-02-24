///////////////////////////////////////////////////////////////////////
// DefensorRenderTypes.h
//
//  Copyright 06/26/2024 Edgar Glowacki.
//
//  Maintained by: Edgar
//
//  NOTES:
//      Specific render types needed for Defensor game
//
//
//  #include "DefensorRenderTypes.h"
//
//////////////////////////////////////////////////////////////////////
//! \file
#pragma once

#include "YagetCore.h"
//#include "Components/SystemsCoordinator.h"
#include "GameSystem/Messaging.h"

namespace defensor::render
{
    //using Messaging = comp::gs::Messaging<std::shared_ptr<char>>;

    //struct EditorComponent { static constexpr int Capacity = 64; };
    //struct EmptyComponent { static constexpr int Capacity = 64; };
    //struct BlankComponent { static constexpr int Capacity = 64; };
    //struct RenderComponent { static constexpr int Capacity = 64; };

    //using RenderEntity = comp::GlobalRowPolicy<RenderComponent*>;

    //using GlobalEntity = comp::GlobalRowPolicy<EditorComponent*>;
    //using Entity = comp::RowPolicy<EmptyComponent*, BlankComponent*>;

    //using GlobalCoordinator = comp::Coordinator<GlobalEntity>;
    //using EntityCoordinator = comp::Coordinator<Entity>;
    //using EditorGameCoordinatorSet = comp::CoordinatorSet<GlobalCoordinator, EntityCoordinator>;

    //using RenderCoordinator = comp::Coordinator<RenderEntity>;
    //using RenderCoordinatorSet = comp::CoordinatorSet<RenderCoordinator>;
}
