///////////////////////////////////////////////////////////////////////
// ClientSystem.h
//
//  Copyright 01/14/2024 Edgar Glowacki.
//
//  Maintained by: Edgar
//
//  NOTES:
//      Manages session connection to server
//
//
//  #include "ClientSystem.h"
//
//////////////////////////////////////////////////////////////////////
//! \file
#pragma once


#include "Components/GameSystem.h"
#include "ClientTypes.h"
#include "ClientComponent.h"

namespace yaget::client
{
    class ClientSystem : public comp::gs::GameSystem<EntityCoordinatorSet, comp::gs::GenerateEndMarker, Messaging, ClientComponent*>
    {
    public:
        ClientSystem(Messaging& messaging, Application& app, EntityCoordinatorSet& coordinatorSet);
        ~ClientSystem() = default;

    private:
        void OnUpdate(comp::Id_t id, const time::GameClock& gameClock, metrics::Channel& channel, const ClientComponent* clientComponent);

        boost::asio::io_context mIoContext;
        network::Ticket_t mConnectionTicket{};
    };
} // namespace yaget::client
