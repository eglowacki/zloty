#include "ServerSystem.h"


//---------------------------------------------------------------------------------------------------------------------
yaget::server::ServerSystem::ServerSystem(Messaging& messaging, Application& app, EntityCoordinatorSet& coordinatorSet)
    : GameSystem("ServerSystem", messaging, app, [this](auto&&... params) {OnUpdate(params...); }, coordinatorSet)
{
    using IdType = IdGameCache::IdType;

    auto& idCache = app.IdCache;

    boost::asio::ip::port_type port = app.Options.find<int>("port", 25000);
    YLOG_INFO("SERV", "Listing on port '%d' for client connections.", port);

    const auto id = idspace::get_burnable(idCache);
    AddComponent<ServerComponent>(id, mIoContext, port);
}


//---------------------------------------------------------------------------------------------------------------------
void yaget::server::ServerSystem::OnUpdate(comp::Id_t id, [[maybe_unused]] const time::GameClock& gameClock, [[maybe_unused]] metrics::Channel& channel, [[maybe_unused]] const ServerComponent* serverComponent)
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
        // operate on serverComponent...
    }
}
