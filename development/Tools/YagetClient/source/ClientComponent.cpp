#include "ClientComponent.h"

#include <boost/asio/connect.hpp>
#include <boost/asio/write.hpp>


//---------------------------------------------------------------------------------------------------------------------
yaget::client::ClientComponent::ClientComponent(comp::Id_t id, boost::asio::io_context& ioContext, boost::asio::ip::tcp::endpoint serverConnection, network::Ticket_t ticket)
    : comp::BaseComponent<>(id)
    , mIoContext(&ioContext)
    , mServerConnection(std::move(serverConnection))
    , mResolver(*mIoContext)
    , mSocket(*mIoContext)
    , mTicket(ticket)
{
    YLOG_INFO("CLNT", "Resolving address to server: '%s:%d'...", mServerConnection.address().to_string().c_str(), mServerConnection.port());
    mResolver.async_resolve(mServerConnection, [this](const boost::system::error_code& error, boost::asio::ip::tcp::resolver::results_type results)
    {
        const auto& endpoint = results.begin()->endpoint();

        if (error)
        {
            YLOG_ERROR("CLNT", "Could not connect to server: '%s:%d'. '%s', '%s'.", endpoint.address().to_string().c_str(), endpoint.port(), error.message().c_str(), error.to_string().c_str());
            return;
        }

        YLOG_INFO("CLNT", "Address to server: '%s:%d' resolved.", endpoint.address().to_string().c_str(), endpoint.port());
        ConnectToSever(results);
    });
}


//---------------------------------------------------------------------------------------------------------------------
void yaget::client::ClientComponent::ConnectToSever(boost::asio::ip::tcp::resolver::results_type resolvedConnection)
{
    YLOG_INFO("CLNT", "Connecting to server: '%s:%d'...", mServerConnection.address().to_string().c_str(), mServerConnection.port());
    boost::asio::async_connect(mSocket, resolvedConnection, [this, resolvedConnection](const boost::system::error_code& error, const boost::asio::ip::tcp::endpoint& endpoint)
    {
        if (error)
        {
            // 10061 - 'No connection could be made because the target machine actively refused it'
            if (error.value() == 10061)
            {
                ConnectToSever(resolvedConnection);
                return;
            }

            YLOG_ERROR("CLNT", "Could not connect to server: '%s:%d'. '%s', '%s'.", endpoint.address().to_string().c_str(), endpoint.port(), error.message().c_str(), error.to_string().c_str());
            return;
        }

        YLOG_INFO("CLNT", "Connected to server: '%s:%d'.", endpoint.address().to_string().c_str(), endpoint.port());

        AuthToSever();
    });
}


//---------------------------------------------------------------------------------------------------------------------
void yaget::client::ClientComponent::AuthToSever()
{
    YLOG_INFO("CLNT", "Getting authorization from server: '%s:%d'...", mServerConnection.address().to_string().c_str(), mServerConnection.port());

    YLOG_INFO("CLNT", "Sending handshake: '%s'.", "GLOW");

    //const uint32_t writeOfset = mWriteOffset;
    // send token id and then 
    std::copy_n(AuthToken.begin(), AuthToken.size(), mNetworkPocketBuffer.begin());
    mWriteOffset += AuthToken.size();

    constexpr auto ticketSize = sizeof(network::Ticket_t);
    std::copy_n(&mTicket, ticketSize, mNetworkPocketBuffer.begin() + mWriteOffset);
    mWriteOffset += ticketSize;

    boost::asio::async_write(mSocket, boost::asio::buffer(mNetworkPocketBuffer.data(), mWriteOffset), boost::asio::transfer_at_least(32), [this](const boost::system::error_code& error, std::size_t bytes_transferred)
    {
        if (error)
        {
            YLOG_ERROR("CLNT", "Could not send Handshake to server. '%s', '%s'.", error.message().c_str(), error.to_string().c_str());
            return;
        }

        YLOG_INFO("CLNT", "Handshake send to server, bytes transferred: '%d'.", bytes_transferred);

        int z =0;
        z;
    });


   //boost::asio::async_write(s,
   //boost::asio::buffer(data, size),
   //boost::asio::transfer_at_least(32),
   //handler);
}

