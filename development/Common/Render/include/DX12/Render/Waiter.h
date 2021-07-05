/////////////////////////////////////////////////////////////////////////
// Waiter.h
//
//  Copyright 06/26/2021 Edgar Glowacki.
//
// NOTES:
//      Condition mutex to synchronize rendering thread with resize
//      and surface change.
//
// #include "Render/Waiter.h"
//
/////////////////////////////////////////////////////////////////////////
//! \file

#pragma once

#include "YagetCore.h"

namespace yaget::render
{
    struct Waiter
    {
    public:
        void Wait();

        void BeginPause();
        void EndPause();

        struct Lock
        {
        public:
            Lock(Waiter& waiter) : mWaiter(waiter)
            {
                mWaiter.BeginPause();
            }

            ~Lock()
            {
                mWaiter.EndPause();
            }

        private:
            Waiter& mWaiter;
        };

    private:
        std::mutex mPauseRenderMutex;
        std::condition_variable mWaitForRenderThread;
        std::atomic_bool mPauseCounter{ false };
        std::condition_variable mRenderPaused;

        int mUsageCounter = 0;
    };

} // namespace yaget::render
