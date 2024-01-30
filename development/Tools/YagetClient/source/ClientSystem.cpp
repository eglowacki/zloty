#include "ClientSystem.h"


//---------------------------------------------------------------------------------------------------------------------
yaget::client::ClientSystem::ClientSystem(Messaging& messaging, Application& app, EntityCoordinatorSet& coordinatorSet)
    : GameSystem("ClientSystem", messaging, app, [this](auto&&... params) {OnUpdate(params...); }, coordinatorSet)
{
    using IdType = IdGameCache::IdType;

    auto& coordinator = GetCoordinator<Entity>();
    auto& idCache = app.IdCache;

    mConnectionTicket = app.Options.find<int>("ticket", 0);
    YLOG_INFO("CLNT", "Connection Ticket: '%d'.", mConnectionTicket);

    Strings ipPortAddress = {"127.0.0.1", "25000"};
    const std::string userAddress = app.Options.find<std::string>("address", "127.0.0.1:25000");

    const auto splitTokens = conv::Split(userAddress, ":", true);
    if (splitTokens.size() == 1)
    {
        ipPortAddress = splitTokens;
        ipPortAddress.push_back(std::to_string(25000));
    }
    else if (splitTokens.size() == 2)
    {
        ipPortAddress = splitTokens;
    }

    //make_address
    boost::system::error_code ec;
    const auto address = boost::asio::ip::make_address(ipPortAddress[0], ec);
    YAGET_ASSERT(!ec.failed(), "Could not make a valid address from: '%s'. %s", ipPortAddress[0].c_str(), ec.to_string().c_str());

    const auto port = conv::Convertor<boost::asio::ip::port_type>::FromString(ipPortAddress[1].c_str());

    boost::asio::ip::tcp::endpoint serverEndPoint{address, port};

    const auto id = idCache.GetId(IdType::Burnable);
    coordinator.AddComponent<ClientComponent>(id, mIoContext, serverEndPoint, mConnectionTicket);
    mItems.insert(id);
}


//---------------------------------------------------------------------------------------------------------------------
yaget::client::ClientSystem::~ClientSystem()
{
    auto& coordinator = GetCoordinator<Entity>();
    coordinator.RemoveItems(mItems);
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
