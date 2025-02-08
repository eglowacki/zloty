//////////////////////////////////////////////////////////////////////
// Performance.h
//
//  Copyright 6/14/2019 Edgar Glowacki
//
//  Maintained by: Edgar
//
//  NOTES:
//      Provides data for performance related work
//
//
//  #include "Metrics/Performance.h"
//
//////////////////////////////////////////////////////////////////////
//! \file
#pragma once

#include "YagetCore.h"
#include "Time/GameClock.h"


namespace yaget::metrics
{
    // data structure representing how a system should run, how much it has, what to do if uses too much time
    // and what to do with remaining computations if uses all budget time 
    struct PerformancePolicy
    {
        // policy on how to proceed if out of budget time
        // drop remaining task
        // defer to next frame
        enum class Policy
        {
            Default,    // let system decide how to handle overflow of computation
            Drop,       // drop any remaining work
            Defer       // defer left over computation work to next tick
        };

        const time::Microsecond_t mBudget = std::numeric_limits<time::Microsecond_t>::max();
        Policy mPolicy = Policy::Default;
    };

} // namespace yaget::metrics
