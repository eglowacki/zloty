﻿///////////////////////////////////////////////////////////////////////
// ServerSystem.h
//
//  Copyright 01/11/2024 Edgar Glowacki.
//
//  Maintained by: Edgar
//
//  NOTES:
//      Manages sessions and connections to all clients/users/players
//
//
//  #include "ServerSystem.h"
//
//////////////////////////////////////////////////////////////////////
//! \file
#pragma once


#include "Components/GameSystem.h"
#include "ServerTypes.h"

namespace yaget::server
{
    class ServerSystem : public comp::gs::GameSystem<EntityCoordinatorSet, comp::gs::GenerateEndMarker, Messaging, ServerComponent*>
    {
    public:
        ServerSystem(Messaging& messaging, Application& app, EntityCoordinatorSet& coordinatorSet);
        ~ServerSystem() = default;

    private:
        void OnUpdate(comp::Id_t id, const time::GameClock& gameClock, metrics::Channel& channel, const ServerComponent* serverComponent);

        boost::asio::io_context mIoContext;
    };
} // namespace yaget::server
