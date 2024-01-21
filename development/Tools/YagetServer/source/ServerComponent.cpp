#include "ServerComponent.h"

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

// m_IoService.run();
yaget::server::ServerComponent::ServerComponent(comp::Id_t id, boost::asio::io_context& ioContext, boost::asio::ip::port_type port)
    : comp::BaseComponent<>(id)
    , mIoContext(ioContext)
    , mEndpoint(boost::asio::ip::tcp::v4(), port)
    , mAcceptor(mIoContext, mEndpoint)
    , mSocket(mIoContext)
{
    YLOG_INFO("SERV", "Listening for connections on port '%d'.", mEndpoint.port());
    //const auto session = mAcceptor.accept(ioContext);
    //YLOG_INFO("SERV", "Connection established with client '%s:%d'.", session.remote_endpoint().address().to_string().c_str(), session.remote_endpoint().port());

    //mAcceptor.async_accept(mSocket, accept_handler);
    mAcceptor.async_accept([](const boost::system::error_code& error, boost::asio::ip::tcp::socket socket)
    {
        if (!error)
        {
            // Accept succeeded.
            int z = 0;
            z;
            YLOG_INFO("SERV", "Connection established with client '%s:%d'.", socket.remote_endpoint().address().to_string().c_str(), socket.remote_endpoint().port());
        }

        int y = 0;
        y;
    });

    //mAcceptor.async_accept(ioContext, [this](auto error, auto socket)
    //{
    //    YLOG_INFO("SERV", "Connection established with client '%s:%d'.", mEndpoint.address().to_string().c_str(), mEndpoint.port());
    //    int z =0;
    //    z;
    //});
}
