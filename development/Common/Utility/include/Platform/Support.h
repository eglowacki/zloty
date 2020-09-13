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

namespace std { class thread; }

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

        // Various Sleep functions
        using SleepPredicate = std::function<bool()>;
        // sleep while SleepPredicate returns true, return from sleep if predicate returns false
        void Sleep(SleepPredicate sleepPredicate);

        void Sleep(time::TimeUnits_t numSleep, time::TimeUnits_t unitType);

        void BusySleep(time::TimeUnits_t numSleep, time::TimeUnits_t unitType);

        bool ParseArgs(const char* commandLine, args::Options& options, std::string* errorMessage);
        bool ParseArgs(args::Options& options, std::string* errorMessage);

        /// Return random number in [from, to] range
        int GetRandom(int from, int to);

        // returns current real time
        // timeUnit - it's one of time::k*Unit like kMicrosecondUni, tetc
        time::TimeUnits_t GetRealTime(time::TimeUnits_t timeUnit);

        // Adjust drift of real time.
        // amount - by what amount, it can be +/-
        // timeUnit - it's one of time::k*Unit like kMicrosecondUnit, etc.
        void AdjustDrift(time::TimeUnits_t amount, time::TimeUnits_t timeUnit);

        inline double GetRealTime()
        {
            time::Raw_t currentTime = GetRealTime(time::kRawUnit);
            return time::FromTo<double>(currentTime, time::kRawUnit, time::kSecondUnit);
        }

        void LogLastError(const std::string& userMessage);
        std::string LastErrorMessage();

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

