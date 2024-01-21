///////////////////////////////////////////////////////////////////////
// ClientCoordinator.h
//
//  Copyright 01/11/2024 Edgar Glowacki.
//
//  Maintained by: Edgar
//
//  NOTES:
//      
//
//
//  #include "ClientCoordinator.h"
//
//////////////////////////////////////////////////////////////////////
//! \file
#pragma once

#include "YagetCore.h"
#include "ClientSystem.h"


namespace yaget::client
{
    namespace internal
    {
        using SystemsCoordinatorE = comp::gs::SystemsCoordinator<EntityCoordinatorSet, Messaging, Application, ClientSystem>;
    }

    class ClientSystemsCoordinator : public internal::SystemsCoordinatorE
    {
    public:
        ClientSystemsCoordinator(Messaging& m, Application& app);
        ~ClientSystemsCoordinator();

    private:
        comp::ItemIds mItems;
        boost::asio::io_context mIoContext;
    };
}
