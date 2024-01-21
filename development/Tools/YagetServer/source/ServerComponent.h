///////////////////////////////////////////////////////////////////////
// ServerComponent.h
//
//  Copyright 01/11/2024 Edgar Glowacki.
//
//  Maintained by: Edgar
//
//  NOTES:
//      Server Components used by server application
//
//
//  #include "ServerComponent.h"
//
//////////////////////////////////////////////////////////////////////
//! \file
#pragma once


#include "Components/Component.h"
#include "Components/ComponentTypes.h"
#include <boost/asio/ip/tcp.hpp>


namespace yaget::server
{
    class ServerComponent : public comp::BaseComponent<>
    {
    public:
        ServerComponent(comp::Id_t id, boost::asio::io_context& ioContext, boost::asio::ip::port_type port);;

        boost::asio::io_context& mIoContext;

    private:
        boost::asio::ip::tcp::endpoint mEndpoint;
        boost::asio::ip::tcp::acceptor mAcceptor;
        boost::asio::ip::tcp::socket mSocket;
    };
}
