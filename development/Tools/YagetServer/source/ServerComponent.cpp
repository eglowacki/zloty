#include "ServerComponent.h"

#include <boost/asio/read.hpp>

//---------------------------------------------------------------------------------------------------------------------
void accept_handler(const boost::system::error_code& error)
{
    if (!error)
    {
        // Accept succeeded.
        int z = 0;
        z;
        YLOG_INFO("SERV", "Connection established with client 'xxx'.");
    }

    int y = 0;
    y;
}


//---------------------------------------------------------------------------------------------------------------------
yaget::server::ServerComponent::ServerComponent(comp::Id_t id, boost::asio::io_context& ioContext, boost::asio::ip::port_type port)
    : comp::BaseComponent<>(id)
    , mIoContext(ioContext)
    , mEndpoint(boost::asio::ip::tcp::v4(), port)
    , mAcceptor(mIoContext, mEndpoint)
    , mSocket(mIoContext)
{
    ListenForSession();
}


//---------------------------------------------------------------------------------------------------------------------
void yaget::server::ServerComponent::ListenForSession()
{
    YLOG_INFO("SERV", "Listening for connections on port '%d'...", mEndpoint.port());
    mAcceptor.async_accept([this](const boost::system::error_code& error, boost::asio::ip::tcp::socket socket)
    {
        if (error)
        {
            const auto endpoint = socket.remote_endpoint();
            YLOG_ERROR("SERV", "Connection with client '%s:%d' could not be established. %s" , endpoint.address().to_string().c_str(), endpoint.port(), error.message());
            return;
        }

        // Accept succeeded.
        try
        {
            const auto remoteEndpoint = socket.remote_endpoint();
            const auto [insertedElement, success] = mPotentialSessions.insert( 
            {
                remoteEndpoint,
                Session
                {
                    .mStatus = Session::Status::Connected,
                    .mSocket = std::move(socket),
                    .mTicket = GetTicket(remoteEndpoint)
                }
            });
            YAGET_ASSERT(success, "Session '%s' already exist in Potential Sessions.", insertedElement->second.EndpointString().c_str());

            YLOG_INFO("SERV", "Connection established with client '%s'.", insertedElement->second.EndpointString().c_str());

            AuthorizeSession(insertedElement->second.mSocket.remote_endpoint());
        }
        catch (const boost::system::system_error& ec)
        {
            const auto errorMessage = ec.what();
            YLOG_ERROR("SERV", "Connection error: '%s'.", errorMessage);
        }

    });
}


//---------------------------------------------------------------------------------------------------------------------
void yaget::server::ServerComponent::AuthorizeSession(boost::asio::ip::tcp::endpoint endpoint)
{
    auto it = mPotentialSessions.find(endpoint);
    if (it != mPotentialSessions.end())
    {
        auto& session = it->second;
        session.mStatus = Session::Status::Auth;

        YLOG_INFO("SERV", "Client connection '%s' request authorized.", session.EndpointString().c_str());

        const auto bufferSize = AuthToken.size() + sizeof(Ticket_t);
        boost::asio::async_read(session.mSocket, boost::asio::buffer(session.mConnectionBuffer.data(), bufferSize), [this, &session, bufferSize, endpoint](boost::system::error_code error, std::size_t size)
        {
            if (!error)
            {
                if (size == bufferSize)
                {
                    const char* authId = session.mConnectionBuffer.data();
                    Ticket_t ticket = static_cast<Ticket_t>(*(session.mConnectionBuffer.data() + AuthToken.size()));

                    if (std::memcmp(authId, AuthToken.data(), AuthToken.size()) == 0 && ticket == session.mTicket)
                    {
                        const auto [insertedElement, success] = mAuthSessions.insert( 
                        {
                            endpoint,
                            std::move(session)
                        });
                        YAGET_ASSERT(success, "Session '%s' already exist in Authorized Sessions.", insertedElement->second.EndpointString().c_str());

                        YLOG_INFO("SERV", "Client connection '%s' validated and approved.", insertedElement->second.EndpointString().c_str());
                    }
                    else
                    {
                        char token[] = {AuthToken[0], AuthToken[1], AuthToken[2], AuthToken[3], '\0'};
                        YLOG_ERROR("SERV", "Client '%s' (Token: '%s', Ticket: '%d') did not get validate and it's not approved for connection. ()", session.EndpointString().c_str(), token, ticket);
                    }

                    mPotentialSessions.erase(endpoint);

                    // EG Note: we need to create a Session object here and tell it which (UDP) port it needs to reconnect to
                    // ...

                    ListenForSession();
                }
            }

            // 2 - 'End of file'
            if (error.value() == 2)
            {
                YLOG_INFO("SERV", "Client '%s' disconnected.", session.EndpointString().c_str());
                mPotentialSessions.erase(endpoint);
                return;
            }
            else if (error)
            {
                YLOG_ERROR("SERV", "Data from Client '%s' for auth did not arrive. %s", session.EndpointString().c_str(), error.message());
                return;
            }
        });
    }
}


//---------------------------------------------------------------------------------------------------------------------
yaget::server::ServerComponent::Ticket_t yaget::server::ServerComponent::GetTicket(boost::asio::ip::tcp::endpoint endpoint) const
{
    return 5;
}
