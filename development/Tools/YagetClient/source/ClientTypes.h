///////////////////////////////////////////////////////////////////////
// ClientTypes.h
//
//  Copyright 01/14/2024 Edgar Glowacki.
//
//  Maintained by: Edgar
//
//  NOTES:
//      Types used by client application
//
//
//  #include "ClientTypes.h"
//
//////////////////////////////////////////////////////////////////////
//! \file
#pragma once

#include "YagetCore.h"
#include "ClientComponent.h"

#include "Components/SystemsCoordinator.h"
#include "GameSystem/Messaging.h"

namespace yaget::client
{
    class ClientSystem;

    using Messaging = comp::gs::Messaging<std::shared_ptr<char>>;

    using Entity = comp::RowPolicy<ClientComponent*>;

    using EntityCoordinator = comp::Coordinator<Entity>;
    using EntityCoordinatorSet = comp::CoordinatorSet<EntityCoordinator>;

    using ClientSystemsCoordinator = comp::gs::SystemsCoordinator<EntityCoordinatorSet, Messaging, Application, ClientSystem>;
}
