///////////////////////////////////////////////////////////////////////
// StagerSystem.h
//
//  Copyright 02/12/2025 Edgar Glowacki.
//
//  Maintained by: Edgar
//
//  NOTES:
//      Provides games specific loading, unloading and lifetime management of Components/Items
//
//
//  #include "StagerSystem.h"
//
//////////////////////////////////////////////////////////////////////
//! \file
#pragma once

#include "DefensorGameTypes.h"
#include "Items/StagerSystem.h"

namespace defensor::game
{
    class DefensorStagerSystem : public yaget::items::StagerSystem<GameCoordinatorSet, Messaging>
    {
    public:
        DefensorStagerSystem(Messaging& messaging, Application& app, GameCoordinatorSet& coordinatorSet)
            : yaget::items::StagerSystem<GameCoordinatorSet, Messaging>("DefensorStagerSystem", messaging, app, coordinatorSet)
        {}
    };

}
