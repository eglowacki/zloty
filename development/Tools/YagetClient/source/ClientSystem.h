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

namespace yaget::client
{
    class ClientSystem : public comp::gs::GameSystem<comp::gs::NoEndMarker, Messaging, ClientComponent*>
    {
    public:
        ClientSystem(Messaging& messaging, Application& app);

    private:
        void OnUpdate(comp::Id_t id, const time::GameClock& gameClock, metrics::Channel& channel, const ClientComponent* clientComponent);
    };
} // namespace yaget::client
