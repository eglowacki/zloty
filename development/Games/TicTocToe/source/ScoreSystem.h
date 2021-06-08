///////////////////////////////////////////////////////////////////////
// BoardSystem.h
//
//  Copyright 10/10/2020 Edgar Glowacki.
//
//  Maintained by: Edgar
//
//  NOTES:
//      This track current board status and generate status report
//
//
//  #include "BoardSystem.h"
//
//////////////////////////////////////////////////////////////////////
//! \file
#pragma once

#include "Components/GameSystem.h"
#include "GameTypes.h"


namespace ttt
{
    class ScoreComponent;

    class ScoreSystem : public yaget::comp::gs::GameSystem<NoEndMarker, Messaging, ScoreComponent*>
    {
    public:
        ScoreSystem(Messaging& messaging, yaget::Application& app);

    private:
        void OnUpdate(yaget::comp::Id_t id, const yaget::time::GameClock& gameClock, yaget::metrics::Channel& channel, ScoreComponent* boardComponent);
    };


}


//inline void ttt::ScoreSystem::Tick(GameCoordinatorSet& coordinatorSet)
//{
//    coordinatorSet;
//}
