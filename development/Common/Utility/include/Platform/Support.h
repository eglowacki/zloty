///////////////////////////////////////////////////////////////////////
// Support.h
//
//  Copyright 07/18/2016 Edgar Glowacki.
//
//  Maintained by: Edgar
//
//  NOTES:
//      Provides platform type code
//
//
//  #include "Platform/Support.h"
//
//////////////////////////////////////////////////////////////////////
//! \file
#pragma once

#include "YagetCore.h"
#include "Time/GameClock.h"
#include <functional>

class std::thread;

namespace yaget
{
    namespace platform                          
    {
        // Thread info used mostly in debugging and performance
        void SetThreadName(const char* threadName, std::thread& t);
        void SetThreadName(const char* threadName, uint32_t t);
        std::string GetThreadName(uint32_t t);
        uint32_t GetThreadId(std::thread& t);
        uint32_t CurrentThreadId();
        std::string GetCurrentThreadName();

        using ThreadNames = std::map<uint32_t, std::string>;
        const ThreadNames& GetThreadNames();

        // Various Sleep functions
        // Keep sleeping while predicate returns true
        using SleepPredicate = std::function<bool()>;

        // sleep while SleepPredicate returns true, return from sleep if predicate returns false
        void Sleep(SleepPredicate sleepPredicate);

        // Allows to specify how long to wait before quit sleep with predicate
        // sleep while SleepPredicate returns true, return from sleep if predicate returns false
        enum class SleepResult { TimedOut, OK };
        SleepResult Sleep(time::TimeUnits_t maxSleepSleep, time::TimeUnits_t unitType, SleepPredicate sleepPredicate);

        void Sleep(time::TimeUnits_t maxSleepSleep, time::TimeUnits_t unitType);

        void BusySleep(time::TimeUnits_t maxSleepSleep, time::TimeUnits_t unitType);

        bool ParseArgs(const char* commandLine, args::Options& options, std::string* errorMessage);
        bool ParseArgs(args::Options& options, std::string* errorMessage);

        /// Return random number in [from, to] range (inclusive-inclusive)
        int GetRandom(int from, int to);

        // returns current real time
        // timeUnit - it's one of time::k*Unit like kMicrosecondUnit, etc.
        time::TimeUnits_t GetRealTime(time::TimeUnits_t timeUnit);

        // Adjust drift of real time.
        // amount - by what amount, it can be +/-
        // timeUnit - it's one of time::k*Unit like kMicrosecondUnit, etc.
        void AdjustDrift(time::TimeUnits_t amount, time::TimeUnits_t timeUnit);

        inline double GetRealTime()
        {
            const time::Raw_t currentTime = GetRealTime(time::kRawUnit);
            return time::FromTo<double>(currentTime, time::kRawUnit, time::kSecondUnit);
        }

        // Return current date and time formatted to format parameter.
        std::string GetCurrentDateTime(const char* format = "%A %B %d, %Y. %T");

        std::string LastErrorMessage();

        void DisregardAttachedDebugger();
        bool IsDebuggerAttached();
        void DebuggerBreak();
        void DebuggerOutput(const std::string& message);

    } // namespace yaget

    namespace rng
    {
        float GetRandom();
        int GetDice(int numberSides);
        float GetRandomRange(float lowValue, float hiValue);

    }

} // namespace yaget

