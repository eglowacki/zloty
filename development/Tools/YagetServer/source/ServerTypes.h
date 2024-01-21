///////////////////////////////////////////////////////////////////////
// ServerTypes.h
//
//  Copyright 01/11/2024 Edgar Glowacki.
//
//  Maintained by: Edgar
//
//  NOTES:
//      Types used by server application
//
//
//  #include "ServerTypes.h"
//
//////////////////////////////////////////////////////////////////////
//! \file
#pragma once

#include "YagetCore.h"
#include "ServerComponent.h"

#include "Components/SystemsCoordinator.h"
#include "GameSystem/Messaging.h"

namespace yaget::server
{
    class ServerSystem;

    using Messaging = comp::gs::Messaging<std::shared_ptr<char>>;

    using Entity = comp::RowPolicy<ServerComponent*>;

    using EntityCoordinator = comp::Coordinator<Entity>;
    using EntityCoordinatorSet = comp::CoordinatorSet<EntityCoordinator>;

    using ServerSystemsCoordinator = comp::gs::SystemsCoordinator<EntityCoordinatorSet, Messaging, Application, ServerSystem>;

}
