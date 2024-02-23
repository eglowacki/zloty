#include "ClientSystem.h"

#include "Network/ErrorHandlers.h"
#include "Network\EndPoints.h"


//---------------------------------------------------------------------------------------------------------------------
yaget::client::ClientSystem::ClientSystem(Messaging& messaging, Application& app, EntityCoordinatorSet& coordinatorSet)
    : GameSystem("ClientSystem", messaging, app, [this](auto&&... params) {OnUpdate(params...); }, coordinatorSet)
{
    using IdType = IdGameCache::IdType;

    auto& idCache = app.IdCache;

    mConnectionTicket = app.Options.find<int>("ticket", 0);
    YLOG_INFO("CLNT", "Connection Ticket: '%d'.", mConnectionTicket);

    const auto& address = app.Options.find<std::string>("address", "127.0.0.1:25000");
    YLOG_INFO("CLNT", "Server address: '%s'.", address.c_str());

    boost::system::error_code ec;
    const auto serverEndPoint = network::CreateEndPoint(address, ec);
    network::ThrowOnError(ec, fmt::format("Could not make a valid address from '{}'", address));
        
    const auto id = idspace::get_burnable(idCache);
    auto* client = AddComponent<ClientComponent>(id, mIoContext, serverEndPoint, mConnectionTicket);

//#define YAGET_TEST_POINTER_TYPES
#ifdef YAGET_TEST_POINTER_TYPES
    const ClientComponent* client2 = nullptr;
    client2 = client;
    (void)client2->Id();
    //client2->CurrentStatus();

    ClientComponent* const client3 = client;
    //client3 = client;
    (void)client3->Id();
    client3->CurrentStatus();

    ClientComponent const *client4 = nullptr;
    client4 = client;
    (void)client4->Id();
    //client4->CurrentStatus();

    const ClientComponent* const client5 = client;
    //client5 = client;
    (void)client5->Id();
    //client5->CurrentStatus();

    int z = 0;
    z;
#endif // YAGET_TEST_POINTER_TYPES

}


//---------------------------------------------------------------------------------------------------------------------
void yaget::client::ClientSystem::OnUpdate(comp::Id_t id, [[maybe_unused]] const time::GameClock& gameClock, [[maybe_unused]] metrics::Channel& channel, [[maybe_unused]] const ClientComponent* clientComponent)
{
    if (id == comp::END_ID_MARKER)
    {
        boost::system::error_code ec;
        mIoContext.poll(ec);
        if (ec)
        {
            int z = 0;
            z;
        }
    }
    else
    {
        // operate on ClientComponent...
    }
}
