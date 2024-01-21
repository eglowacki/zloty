#include "ServerSystem.h"


yaget::server::ServerSystem::ServerSystem(Messaging& messaging, Application& app)
    : GameSystem("ServerSystem", messaging, app, [this](auto&&... params) {OnUpdate(params...); })
{
    //using IdType = IdGameCache::IdType;

    //auto& coordinator = GetCoordinator<Entity>();
    //auto& idCache = app.IdCache;

    //boost::asio::ip::port_type port = app.Options.find<int>("port", 25000);

    //const auto id = idCache.GetId(IdType::Burnable);
    //coordinator.AddComponent<ServerComponent>(id, mIoContext, port);
    //mItems.insert(id);
}

void yaget::server::ServerSystem::OnUpdate([[maybe_unused]] comp::Id_t id, [[maybe_unused]] const time::GameClock& gameClock, [[maybe_unused]] metrics::Channel& channel, [[maybe_unused]] const ServerComponent* serverComponent)
{
    boost::system::error_code ec;

    serverComponent->mIoContext.poll(ec);
    if (ec)
    {
        int z = 0;
        z;
    }
    int z = 0;
    z;
}
