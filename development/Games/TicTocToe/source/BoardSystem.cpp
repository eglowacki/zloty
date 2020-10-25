#include "BoardSystem.h"
#include "BoardComponent.h"


ttt::BoardSystem::BoardSystem()
    : GameSystem("BoardSystem", [this](auto&&... params) {OnUpdate(params...); })
{
}
//
//void ttt::BoardSystem::OnUpdate(yaget::comp::Id_t /*id*/, const yaget::time::GameClock& /*gameClock*/, yaget::metrics::Channel& /*channel*/, BoardComponent* /*boardComponent*/)
//{
//}
void ttt::BoardSystem::OnUpdate(yaget::comp::Id_t id, BoardComponent* boardComponent)
{
    id;
    boardComponent;
}
