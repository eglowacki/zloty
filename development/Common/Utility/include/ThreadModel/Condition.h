/////////////////////////////////////////////////////////////////////////
// Condition.h
//
//  Copyright 5/15/2018 Edgar Glowacki.
//
// NOTES:
//      Condition variable with Trigger and Wait
//
//
// #include "ThreadModel/Condition.h"
//
/////////////////////////////////////////////////////////////////////////
//! \file

#pragma once

#include "YagetCore.h"
#include "Time/GameClock.h"

#include <mutex>
#include <chrono>

namespace yaget
{
    namespace mt
    {
        //! Condition class to handle condition variable
        //! Usage:
        //!     Waiting thread:     Condition.Wait();
        //!     Triggering thread:  Condition.Trigger();
        class Condition : public yaget::Noncopyable<Condition>
        {
        public:
            void Trigger()
            {
                std::lock_guard<std::mutex> locker(mMutex);
                mRelease = true;
                mCondition.notify_one();
            }

            // will wait on Trigger() to be called (from other thread)
            // numSleep allows us to just wait until that time pass before it get's triggered.
            void Wait(time::TimeUnits_t numSleep = 0, time::TimeUnits_t unitType = time::kMilisecondUnit)
            {
                std::unique_lock<std::mutex> locker(mMutex);
                if (numSleep)
                {
                    using namespace std::chrono_literals;

                    if (unitType == time::kMicrosecondUnit)
                    {
                        mCondition.wait_for(locker, numSleep * 1us, [this] { return mRelease; });
                    }
                    else if (unitType == time::kMilisecondUnit)
                    {
                        mCondition.wait_for(locker, numSleep * 1ms, [this] { return mRelease; });
                    }
                }
                else
                {
                    mCondition.wait(locker, [this] { return mRelease; });
                }

                mRelease = false;
            }

            void Reset()
            {
                std::lock_guard<std::mutex> locker(mMutex);
                mRelease = false;
            }

        private:
            std::mutex mMutex;
            std::condition_variable mCondition;
            bool mRelease = false;
        };

    } // namespace mt
} // namespace yaget


