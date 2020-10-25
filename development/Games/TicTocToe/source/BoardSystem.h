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


#include "Components/CoordinatorSet.h"
#include "Components/GameSystem.h"
#include "GameCoordinator.h"

namespace ttt
{
    class BoardComponent;

    class BoardSystem : public yaget::comp::gs::GameSystem<yaget::NoEndMarkerGlobal, BoardComponent*>
    {
    public:
        BoardSystem();

    private:
        void OnUpdate(yaget::comp::Id_t id, BoardComponent* boardComponent);
    };

}
