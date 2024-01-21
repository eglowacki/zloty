#include "ClientSystem.h"


yaget::client::ClientSystem::ClientSystem(Messaging& messaging, Application& app)
    : GameSystem("ClientSystem", messaging, app, [this](auto&&... params) {OnUpdate(params...); })
{
}

void yaget::client::ClientSystem::OnUpdate([[maybe_unused]] comp::Id_t id, [[maybe_unused]] const time::GameClock& gameClock, [[maybe_unused]] metrics::Channel& channel, [[maybe_unused]] const ClientComponent* clientComponent)
{
    boost::system::error_code ec;

    clientComponent->mIoContext.poll(ec);
    int z = 0;
    z;
}
