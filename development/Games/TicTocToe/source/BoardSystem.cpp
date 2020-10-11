#include "BoardSystem.h"

YAGET_COMPILE_SUPRESS_START(4100, "'': unreferenced local variable")

ttt::BoardSystem::BoardSystem()
    : GameSystem("BoardSystem", [this](auto&&... params) {OnUpdate(params...); })
{
}

void ttt::BoardSystem::OnUpdate(yaget::comp::Id_t id, const yaget::time::GameClock& gameClock, yaget::metrics::Channel& channel, BoardComponent* boardComponent)
{
}

YAGET_COMPILE_SUPRESS_END