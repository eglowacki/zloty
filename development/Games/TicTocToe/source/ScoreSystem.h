///////////////////////////////////////////////////////////////////////
// BoardSystem.h
//
//  Copyright 10/10/2020 Edgar Glowacki.
//
//  Maintained by: Edgar
//
//  NOTES:
//      System which iterates over board every tick and updates it's state
//
//
//  #include "BoardSystem.h"
//
//////////////////////////////////////////////////////////////////////
//! \file
#pragma once

#include "Components/GameSystem.h"
//#include "GameCoordinator.h"


namespace ttt
{
    class ScoreComponent;

    class ScoreSystem : public yaget::comp::gs::GameSystem<yaget::NoEndMarkerGlobal, ScoreComponent*>
    {
    public:
        ScoreSystem();

    private:
        void OnUpdate(yaget::comp::Id_t id, ScoreComponent* boardComponent);
    };


}


//inline void ttt::ScoreSystem::Tick(GameCoordinatorSet& coordinatorSet)
//{
//    coordinatorSet;
//}
