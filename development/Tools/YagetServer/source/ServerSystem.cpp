#include "ServerSystem.h"


//---------------------------------------------------------------------------------------------------------------------
yaget::server::ServerSystem::ServerSystem(Messaging& messaging, Application& app, EntityCoordinatorSet& coordinatorSet)
    : GameSystem("ServerSystem", messaging, app, [this](auto&&... params) {OnUpdate(params...); }, coordinatorSet)
{
    using IdType = IdGameCache::IdType;

    // EG: note we should explore meta-template to expose AddComponent from GameSystem class <========
    auto& coordinator = GetCoordinator<Entity>();
    auto& idCache = app.IdCache;

    boost::asio::ip::port_type port = app.Options.find<int>("port", 25000);

    try
    {
        const auto id = idCache.GetId(IdType::Burnable);
        coordinator.AddComponent<ServerComponent>(id, mIoContext, port);
        mItems.insert(id);
    }
    catch (const boost::system::system_error& ec)
    {
        const auto errorMessage = ec.what();
        YLOG_INFO("SERV", "Networking error: '%s'.", errorMessage);
    }
}


//---------------------------------------------------------------------------------------------------------------------
yaget::server::ServerSystem::~ServerSystem()
{
    auto& coordinator = GetCoordinator<Entity>();
    coordinator.RemoveItems(mItems);
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
