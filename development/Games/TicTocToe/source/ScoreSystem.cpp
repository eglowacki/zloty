#include "ScoreSystem.h"


ttt::ScoreSystem::ScoreSystem()
    : GameSystem("ScoreSystem", [this](auto&&... params) {OnUpdate(params...); })
{
}



void ttt::ScoreSystem::OnUpdate(yaget::comp::Id_t id, ScoreComponent* boardComponent)
{
    id;
    boardComponent;
}
