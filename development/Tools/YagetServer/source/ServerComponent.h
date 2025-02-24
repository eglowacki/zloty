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
#include "NetworkTypes.h"

#include <boost/asio/ip/tcp.hpp>


namespace yaget::server
{
    class ServerComponent : public comp::BaseComponent<>
    {
    public:
        ServerComponent(comp::Id_t id, boost::asio::io_context& ioContext, boost::asio::ip::port_type port);;

    private:
        void ListenForSession();
        void AuthorizeSession(boost::asio::ip::tcp::endpoint endpoint);

        network::Ticket_t GetTicket(boost::asio::ip::tcp::endpoint endpoint) const;

        using ConnectionBuffer = std::array<char, 64>;
        struct Session
        {
            enum class Status
            {
                Closed,
                Connected,
                Auth,
                Open
            };

            Status mStatus = Status::Closed;
            boost::asio::ip::tcp::socket mSocket;

            ConnectionBuffer mConnectionBuffer;
            network::Ticket_t mTicket{};

            std::string EndpointString() const
            {
                return fmt::format("{}:{}", mSocket.remote_endpoint().address().to_string(), mSocket.remote_endpoint().port());
            }
        };

        boost::asio::io_context& mIoContext;
        boost::asio::ip::tcp::endpoint mEndpoint;
        boost::asio::ip::tcp::acceptor mAcceptor;
        boost::asio::ip::tcp::socket mSocket;

        using Sessions = std::map<boost::asio::ip::tcp::endpoint, Session>;

        // EG: note add mutex here for Sessions
        Sessions mPotentialSessions;
        Sessions mAuthSessions;

        const std::array<char, 4> AuthToken{'G', 'L', 'O', 'W'};
    };
}
