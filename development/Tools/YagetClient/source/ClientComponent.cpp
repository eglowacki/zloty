#include "ClientComponent.h"

#include <boost/asio/connect.hpp>
//#include <boost/asio.hpp>


yaget::client::ClientComponent::ClientComponent(comp::Id_t id, boost::asio::io_context& ioContext, boost::asio::ip::tcp::endpoint serverConnection)
    : comp::BaseComponent<>(id)
    , mIoContext(ioContext)
    , mServerConnection(std::move(serverConnection))
    , mResolver(mIoContext)
    //, mAcceptor(ioContext, mServerConnection)
    , mSocket(mIoContext)
{
    YLOG_INFO("CLNT", "Resolving address to server: '%s:%d'...", mServerConnection.address().to_string().c_str(), mServerConnection.port());
    const auto resolvedConnection = mResolver.resolve(mServerConnection);
    //YLOG_INFO("CLNT", "Starting connection to server: '%s:%d'...", mServerConnection.address().to_string().c_str(), mServerConnection.port());

    try
    {
        YLOG_INFO("CLNT", "Connecting to server: '%s:%d'...", mServerConnection.address().to_string().c_str(), mServerConnection.port());
        const auto connection = boost::asio::connect(mSocket, resolvedConnection);
        YLOG_INFO("CLNT", "Connected to server: '%s:%d'...", connection.address().to_string().c_str(), connection.port());
    }
    catch (const boost::system::system_error& ec)
    {
        const auto message = ec.what();
        YLOG_ERROR("CLNT", "Error while connecting to server: '%s:%d'. %s", mServerConnection.address().to_string().c_str(), mServerConnection.port(), message);

    }
    //mSocket.async_connect()
    //boost::asio::async_connect(mSocket, mResolver.)
    //mAcceptor.async_accept(ioContext, [this](auto error, auto socket)
    //{
        int z =0;
        z;
    //});
}
