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
#include <mutex>

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

            void Wait()
            {
                std::unique_lock<std::mutex> locker(mMutex);
                mCondition.wait(locker, [this] { return mRelease; });
                mRelease = false;
            }

        private:
            std::mutex mMutex;
            std::condition_variable mCondition;
            bool mRelease = false;
        };

    } // namespace mt
} // namespace yaget


