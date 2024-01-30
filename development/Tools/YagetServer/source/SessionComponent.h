///////////////////////////////////////////////////////////////////////
// SessionComponent.h
//
//  Copyright 01/25/2024 Edgar Glowacki.
//
//  Maintained by: Edgar
//
//  NOTES:
//      Session Component represents data connection between this server and client
//
//
//  #include "SessionComponent.h"
//
//////////////////////////////////////////////////////////////////////
//! \file
#pragma once


#include "Components/Component.h"
#include "Components/ComponentTypes.h"
#include <boost/asio/ip/tcp.hpp>


namespace yaget::server
{
    class SessionComponent : public comp::BaseComponent<>
    {
    public:
        using Ticket_t = uint32_t;

        SessionComponent(comp::Id_t id, boost::asio::io_context& ioContext);

    private:
        const boost::asio::io_context* mIoContext;
    };
}
