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
    using Messaging = yaget::comp::gs::Messaging<std::shared_ptr<char>>;

    struct EditorComponent { static constexpr int Capacity = 64; };
    struct EmptyComponent { static constexpr int Capacity = 64; };
    struct RenderComponent { static constexpr int Capacity = 64; };

    struct RenderEntity : yaget::comp::RowPolicy<RenderComponent*>
    {
        using AutoCleanup = bool;
        using Global = bool;
    };

    struct GlobalEntity : yaget::comp::RowPolicy<EditorComponent*>
    {
        using AutoCleanup = bool;
        using Global = bool;
    };

    struct Entity : yaget::comp::RowPolicy<EmptyComponent*>
    {
        using AutoCleanup = bool;
    };

    using GlobalCoordinator = yaget::comp::Coordinator<GlobalEntity>;
    using EntityCoordinator = yaget::comp::Coordinator<Entity>;
    using EditorGameCoordinatorSet = yaget::comp::CoordinatorSet<GlobalCoordinator, EntityCoordinator>;

    using RenderCoordinator = yaget::comp::Coordinator<RenderEntity>;
    using RenderCoordinatorSet = yaget::comp::CoordinatorSet<RenderCoordinator>;
}
