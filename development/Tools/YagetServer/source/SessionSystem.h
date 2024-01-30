///////////////////////////////////////////////////////////////////////
// SessionSystem.h
//
//  Copyright 01/24/2024 Edgar Glowacki.
//
//  Maintained by: Edgar
//
//  NOTES:
//      Manages sessions between clients and this server
//
//
//  #include "SessionSystem.h"
//
//////////////////////////////////////////////////////////////////////
//! \file
#pragma once


#include "Components/GameSystem.h"
#include "ServerTypes.h"

namespace yaget::server
{
    class SessionSystem : public comp::gs::GameSystem<EntityCoordinatorSet, comp::gs::GenerateEndMarker, Messaging, SessionComponent*>
    {
    public:
        SessionSystem(Messaging& messaging, Application& app, EntityCoordinatorSet& coordinatorSet);
        ~SessionSystem();

    private:
        void OnUpdate(comp::Id_t id, const time::GameClock& gameClock, metrics::Channel& channel, const SessionComponent* sessionComponent);

        comp::ItemIds mItems;
        //boost::asio::io_context mIoContext;
    };
} // namespace yaget::server
