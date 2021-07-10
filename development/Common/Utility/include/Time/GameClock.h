///////////////////////////////////////////////////////////////////////
//  GameClock.h
//
//  Copyright 07/25/2016 Edgar Glowacki.
//
//  Maintained by: Edgar
//
//  NOTES:
//     Game clock class
//
//
//  #include "Time/GameClock.h"
//
//////////////////////////////////////////////////////////////////////
//! \file
#pragma once

#include "YagetCore.h"
#include <atomic>

namespace yaget::time
{
    constexpr uint64_t INVALID_TICK_COUNTER = std::numeric_limits<uint64_t>::max();
    constexpr uint64_t INVALID_TIME = INVALID_TICK_COUNTER;
    using TimeUnits_t = int64_t;
    constexpr TimeUnits_t kRawUnit          = 1;
    constexpr TimeUnits_t kMicrosecondUnit  = 10;
    constexpr TimeUnits_t kMilisecondUnit   = 10000;
    constexpr TimeUnits_t kSecondUnit       = 10000000;

    using Raw_t                         = TimeUnits_t;
    using Microsecond_t                 = TimeUnits_t;
    using Milisecond_t                  = TimeUnits_t;

    // All clock resolutions are in microseconds. This uses platform GetRealTime functions for real times.
    //
    // Framework calls Tick with some delta step time to advance clock, and then framework calls other systems to process game logic.
    // During calls to game logic systems GameClock will return the same time.
    // 
    // Before first time usage, call GameClock.Resync() method, only if you created GameClock earlier and want to syncronise game time.
    // During game loop, call GameClock.Tick(your_delta_time)
    class GameClock
    {
    public:
        GameClock();

        // Every logic loop (fixed), this is incremented by fixed dt. This value should stay as close to Game time as it can
        Microsecond_t GetLogicTime() const;
        // Current delta time, this will be the last value called to Tick and will stay the same for the frame duration
        Microsecond_t GetDeltaTime() const;
        // Helper method to return delta time in seconds.
        float GetDeltaTimeSecond() const;

        // Resets the mLogicTime to current game time, used only at the start of the game loop for the first time, or long pauses
        void Resync();

        // Should only be called during logic tick with that logic step value, and never outside
        void Tick(Microsecond_t deltaTime);

        //Current tick counter, increment by calls to Tick(...) method
        uint64_t GetTickCounter() const { return mTickCounter; }
        Microsecond_t GetRealTime() const;

    private:
        std::atomic<Microsecond_t> mLogicTime{ 0 };
        std::atomic<Microsecond_t> mDeltaTime{ 0 };
        std::atomic_uint64_t mTickCounter{ 0 };
        std::atomic<Microsecond_t> mResetTime{ 0 };
    };

    template <typename T, typename V>
    constexpr T FromTo(V value, TimeUnits_t from, TimeUnits_t to)
    {
        T newValue = static_cast<T>((static_cast<double>(value) * from) / to);
        return newValue;
    }

    // Helper function to return delta time per one tick based on frames (Hz)
    inline constexpr Microsecond_t GetDeltaTime(uint32_t frames)
    {
        return FromTo<Microsecond_t>(1.0f / frames, time::kSecondUnit, time::kMicrosecondUnit);
    }

    constexpr uint32_t kFrames_30 = 30;
    constexpr uint32_t kFrames_60 = 60;
    constexpr uint32_t kFrames_144 = 144;

    constexpr Microsecond_t kDeltaTime_30 = GetDeltaTime(kFrames_30);
    constexpr Microsecond_t kDeltaTime_60 = GetDeltaTime(kFrames_60);
    constexpr Microsecond_t kDeltaTime_144 = GetDeltaTime(kFrames_144);
} // namespace yaget::time

