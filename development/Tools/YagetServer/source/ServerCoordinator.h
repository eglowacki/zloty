///////////////////////////////////////////////////////////////////////
// ServerCoordinator.h
//
//  Copyright 01/11/2024 Edgar Glowacki.
//
//  Maintained by: Edgar
//
//  NOTES:
//      
//
//
//  #include "ServerCoordinator.h"
//
//////////////////////////////////////////////////////////////////////
//! \file
#pragma once

#include "YagetCore.h"
#include "ServerSystem.h"


namespace yaget::server
{
    namespace internal
    {
        using SystemsCoordinatorE = comp::gs::SystemsCoordinator<EntityCoordinatorSet, Messaging, Application, ServerSystem>;
    }

    class ServerSystemsCoordinator : public internal::SystemsCoordinatorE
    {
    public:
        ServerSystemsCoordinator(Messaging& m, Application& app);
        ~ServerSystemsCoordinator() = default;
    };
}
