#include "ServerCoordinator.h"
#include <boost/asio.hpp>

//-------------------------------------------------------------------------------------------------
yaget::server::ServerSystemsCoordinator::ServerSystemsCoordinator(Messaging& m, Application& app)
    : internal::SystemsCoordinatorE(m, app)
{
    //using IdType = IdGameCache::IdType;

    //auto& coordinator = GetCoordinator<Entity>();
    //auto& idCache = app.IdCache;

    //boost::asio::ip::port_type port = app.Options.find<int>("port", 25000);

    //const auto id = idCache.GetId(IdType::Burnable);
    //coordinator.AddComponent<ServerComponent>(id, mIoContext, port);
    //mItems.insert(id);
}


//-------------------------------------------------------------------------------------------------
yaget::server::ServerSystemsCoordinator::~ServerSystemsCoordinator()
{
    //auto& coordinator = GetCoordinator<Entity>();
    //coordinator.RemoveItems(mItems);
}