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
    using DefensorStagerSystem = yaget::items::StagerSystem<GameCoordinatorSet, Messaging>;

}
