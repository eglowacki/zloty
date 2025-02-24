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
#include "NetworkTypes.h"

#include <boost/asio/ip/tcp.hpp>


namespace yaget::client
{
    class ClientComponent : public comp::BaseComponent<>
    {
    public:
        ClientComponent(comp::Id_t id, boost::asio::io_context& ioContext, boost::asio::ip::tcp::endpoint serverConnection, network::Ticket_t ticket);;

        void CurrentStatus()
        {
            mStatus = Status::Closed;
        }

    private:
        void ConnectToSever(boost::asio::ip::tcp::resolver::results_type resolvedConnection);
        void AuthToSever();

        enum class Status
        {
            Closed,
            Connecting,
            Connected
        };

        boost::asio::io_context* mIoContext{};
        boost::asio::ip::tcp::endpoint mServerConnection;
        boost::asio::ip::tcp::resolver mResolver;
        boost::asio::ip::tcp::socket mSocket;
        network::Ticket_t mTicket{};

        Status mStatus = Status::Closed;    // do we really need this?

        const std::array<char, 4> AuthToken{'G', 'L', 'O', 'W'};

        size_t mWriteOffset = 0;
        std::array<char, 128> mNetworkPocketBuffer{};
    };
}
