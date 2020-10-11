#include "ScoreSystem.h"

YAGET_COMPILE_SUPRESS_START(4100, "'': unreferenced local variable")

ttt::ScoreSystem::ScoreSystem()
    : GameSystem("BoardSystem", [this](auto&&... params) {OnUpdate(params...); })
{
}

void ttt::ScoreSystem::OnUpdate(yaget::comp::Id_t id, const yaget::time::GameClock& gameClock, yaget::metrics::Channel& channel, ScoreComponent* scoreComponent)
{
}

YAGET_COMPILE_SUPRESS_END
