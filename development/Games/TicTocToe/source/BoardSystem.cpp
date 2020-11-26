#include "BoardSystem.h"
#include "BoardComponent.h"


ttt::BoardSystem::BoardSystem(Messaging& messaging)
    : GameSystem("BoardSystem", messaging, [this](auto&&... params) {OnUpdate(params...); })
{
    int z = 0;
    z;
}


void ttt::BoardSystem::OnUpdate(yaget::comp::Id_t id, const yaget::time::GameClock& gameClock, yaget::metrics::Channel& channel, BoardComponent* boardComponent)
{
    id;
    gameClock;
    channel;
    boardComponent;
}
