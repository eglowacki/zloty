///////////////////////////////////////////////////////////////////////
//  DeltaClock.h
//
//  Copyright 05/13/2026 Edgar Glowacki.
//
//  Maintained by: Edgar
//
//  NOTES:
//     Delta Game clock class
//
//
//  #include "Time/DeltaClock.h"
//
//////////////////////////////////////////////////////////////////////
//! \file
#pragma once

#include "YagetCore.h"
#include "Time/GameClock.h"

namespace yaget::time
{
    class DeltaClock
    {
    public:
        DeltaClock(double deltaTimeFrequency);

        bool IsDeltaTimePassed();

    private:
        double mDeltaTimeFrequency{};
        double mLastUpdateTime{};
        double mDeltaLeftOver{ 0 };
    };
    
}

