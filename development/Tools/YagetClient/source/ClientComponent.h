///////////////////////////////////////////////////////////////////////
// ClientComponent.h
//
//  Copyright 01/14/2024 Edgar Glowacki.
//
//  Maintained by: Edgar
//
//  NOTES:
//      Client Components used by server application
//
//
//  #include "ClientComponent.h"
//
//////////////////////////////////////////////////////////////////////
//! \file
#pragma once


#include "Components/Component.h"
#include "Components/ComponentTypes.h"
#include <boost/asio/ip/tcp.hpp>


namespace yaget::client
{
    class ClientComponent : public comp::BaseComponent<>
    {
    public:
        ClientComponent(comp::Id_t id, boost::asio::io_context& ioContext, boost::asio::ip::tcp::endpoint serverConnection);;

    private:
        boost::asio::io_context& mIoContext;
        boost::asio::ip::tcp::endpoint mServerConnection;
        boost::asio::ip::tcp::resolver mResolver;
        boost::asio::ip::tcp::socket mSocket;
    };
}
