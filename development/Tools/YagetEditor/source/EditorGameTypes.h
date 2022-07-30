///////////////////////////////////////////////////////////////////////
// EditorGameTypes.h
//
//  Copyright 06/06/2021 Edgar Glowacki.
//
//  Maintained by: Edgar
//
//  NOTES:
//      Specific game types needed for editor
//
//
//  #include "EditorGameTypes.h"
//
//////////////////////////////////////////////////////////////////////
//! \file
#pragma once

#include "YagetCore.h"
#include "Components/SystemsCoordinator.h"
#include "GameSystem/Messaging.h"

namespace yaget::editor
{
    using Messaging = comp::gs::Messaging<std::shared_ptr<char>>;

    struct EditorComponent { static constexpr int Capacity = 64; };
    struct EmptyComponent { static constexpr int Capacity = 64; };
    struct BlankComponent { static constexpr int Capacity = 64; };
    struct RenderComponent { static constexpr int Capacity = 64; };

    using RenderEntity = comp::GlobalRowPolicy<RenderComponent*>;

    using GlobalEntity = comp::GlobalRowPolicy<EditorComponent*>;
    using Entity = comp::RowPolicy<EmptyComponent*, BlankComponent*>;

    using GlobalCoordinator = comp::Coordinator<GlobalEntity>;
    using EntityCoordinator = comp::Coordinator<Entity>;
    using EditorGameCoordinatorSet = comp::CoordinatorSet<GlobalCoordinator, EntityCoordinator>;

    using RenderCoordinator = comp::Coordinator<RenderEntity>;
    using RenderCoordinatorSet = comp::CoordinatorSet<RenderCoordinator>;
}
