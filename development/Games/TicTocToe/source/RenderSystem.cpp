#include "RenderSystem.h"

ttt::RenderSystem::RenderSystem()
    : GameSystem("RenderSystem", [this](auto&&... params) {OnUpdate(params...); })
{
    int z = 0;
    z;
}

void ttt::RenderSystem::OnUpdate(yaget::comp::Id_t id, const yaget::time::GameClock& gameClock, yaget::metrics::Channel& channel, RenderComponent* renderComponent)
{
    id;
    gameClock;
    channel;
    renderComponent;
}
