//////////////////////////////////////////////////////////////////////
// RenderWorldSystem.h
//
//  Copyright 7/17/2019 Edgar Glowacki
//
//  Maintained by: Edgar
//
//  NOTES:
//      Helpers and defines for ponger game render systems and it's data
//      
//
//
//  #include "Ponger/RenderWorldSystem.h"
//
//////////////////////////////////////////////////////////////////////
//! \file
#pragma once

#include "YagetCore.h"
#include "Components/Coordinator.h"
#include "Components/GameCoordinator.h"
#include "Components/GameSystem.h"
#include "Ponger/PongerComponents.h"
#include "Debugging/Primitives.h"

namespace yaget::render { class LineComponent; }

namespace ponger
{
    using namespace yaget;

    struct DebugDrawComponent : Noncopyable<DebugDrawComponent>
    {
        static constexpr int Capacity = 64;

        using Line = render::primitives::Line;
        using Lines = render::primitives::Lines;
        Lines mLines;
    };

    // rendering components representing entity and global render only component respectively
    using RenderEntity = comp::RowPolicy<ponger::DebugDrawComponent*>;
    using GlobalRenderEntity = comp::RowPolicy<render::LineComponent*>;

    using RenderPolicy = comp::CoordinatorPolicy<ponger::RenderEntity, ponger::GlobalRenderEntity>;

    // this collects all lines from DebugDrawComponent and fills in LineComponent data
    using LineCollectorSystem = comp::GameSystem<yaget::EndMarkerEntity, ponger::DebugDrawComponent*>;
    // render all lines accumulated in LineComponent data from LineCollectorSystem
    using LineRendererSystem = yaget::comp::GameSystem<yaget::NoEndMarkerGlobal, render::LineComponent*>;

    using RenderSystemCoordinator = yaget::GameCoordinator<ponger::RenderPolicy, ponger::LineCollectorSystem*, ponger::LineRendererSystem*>;

} // namespace ponger
