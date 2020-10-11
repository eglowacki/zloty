///////////////////////////////////////////////////////////////////////
// GameCoordinator.h
//
//  Copyright 10/10/2020 Edgar Glowacki.
//
//  Maintained by: Edgar
//
//  NOTES:
//      
//
//
//  #include "GameCoordinator.h"
//
//////////////////////////////////////////////////////////////////////
//! \file
#pragma once

#include "GameTypes.h"


namespace ttt
{
    class BoardSystem;
    class ScoreSystem;

    using GameCoordinator = yaget::GameCoordinator<GamePolicy, BoardSystem*, ScoreSystem*>;

}

