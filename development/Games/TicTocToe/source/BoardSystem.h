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


namespace ttt
{
    class BoardComponent;

    class BoardSystem : public yaget::comp::GameSystem<yaget::NoEndMarkerGlobal, BoardComponent*>
    {
    public:
        BoardSystem();

    private:
        void OnUpdate(yaget::comp::Id_t id, const yaget::time::GameClock& gameClock, yaget::metrics::Channel& channel, BoardComponent* boardComponent);

    };

}

