﻿#include "ScoreSystem.h"


ttt::ScoreSystem::ScoreSystem(Messaging& messaging, yaget::Application& app)
    : GameSystem("ScoreSystem", messaging, app, [this](auto&&... params) {OnUpdate(params...); })
{
}



void ttt::ScoreSystem::OnUpdate(yaget::comp::Id_t id, const yaget::time::GameClock& gameClock, yaget::metrics::Channel& channel, ScoreComponent* boardComponent)
{
    id;
    gameClock;
    channel;
    boardComponent;
}
