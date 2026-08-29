///////////////////////////////////////////////////////////////////////
// DefensorGameTypes.h
//
//  Copyright 06/26/2024 Edgar Glowacki.
//
//  Maintained by: Edgar
//
//  NOTES:
//      Specific game types needed for Defensor game
//      "python + jinja2 for a c++ code" search term
//
//
//  #include "DefensorGameTypes.h"
//
//////////////////////////////////////////////////////////////////////
//! \file
#pragma once

#include "Components/BulletComponent.h"
#include "Components/ComponentTypes.h"
#include "Components/InputComponent.h"
#include "Components/LocationComponent.h"
#include "Components/MaterialComponent.h"
#include "Components/MenuComponent.h"
#include "Components/MenuScreenComponent.h"
#include "Components/NameComponent.h"
#include "Components/PayloadStager.h"
#include "Components/ScriptComponent.h"
#include "Components/SystemsCoordinator.h"
#include "Components/UnitComponent.h"
#include "Components/TextComponent.h"
#include "Components/VelocityComponent.h"
#include "Items/StageComponent.h"
#include "Renders/RenderComponent.h"
#include "Renders/SceneComponent.h"
#include "Streams/Buffers.h"


namespace defensor::game
{
    using namespace yaget;

    using Messaging = comp::PayloadStager<io::MessagingBuffer>;
    using MessagingPayload = Messaging::Payload;

    using GlobalEntity = comp::GlobalRowPolicy<
        comp::MenuComponent*, 
        items::StageComponent*
    >;

    using Entity = comp::RowPolicy<
        comp::LocationComponent3*,
        comp::InputComponent*,
        comp::UnitComponent*,
        comp::ScriptComponent*,
        comp::NameComponent*,
        comp::VelocityComponent*,
        comp::MaterialComponent*,
        comp::BulletComponent*,
        comp::TextComponent*,
        comp::MenuScreenComponent*
    >;

    using GlobalCoordinator = comp::Coordinator<GlobalEntity>;
    using EntityCoordinator = comp::Coordinator<Entity>;

    using GameCoordinatorSet = comp::CoordinatorSet<GlobalCoordinator, EntityCoordinator>;
}

namespace defensor::render
{
    using namespace yaget;

    using Messaging = game::Messaging;
            
    using GlobalEntity = comp::GlobalRowPolicy<SceneComponent*>;
    using Entity = comp::RowPolicy<RenderComponent*>;

    using GlobalCoordinator = comp::Coordinator<GlobalEntity>;
    using EntityCoordinator = comp::Coordinator<Entity>;

    using RenderCoordinatorSet = comp::CoordinatorSet<GlobalCoordinator, EntityCoordinator>;
}
