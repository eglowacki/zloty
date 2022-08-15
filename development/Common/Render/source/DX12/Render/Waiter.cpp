#include "Render/Waiter.h"
#include "Platform/Support.h"


//-------------------------------------------------------------------------------------------------
void yaget::render::Waiter::Wait()
{
    if (mPauseCounter == true)
    {
        YLOG_NOTICE("DEVI", "Waiter - We are requested to pause from Thread: '%s'. Stopping.", yaget::platform::GetCurrentThreadName().c_str());
        mWaitForRenderThread.notify_one();
        std::unique_lock<std::mutex> locker(mPauseRenderMutex);
        mRenderPaused.wait(locker);
        YLOG_NOTICE("DEVI", "Waiter - Resuming Render from Thread: '%s'.", yaget::platform::GetCurrentThreadName().c_str());
    }
}


//-------------------------------------------------------------------------------------------------
void yaget::render::Waiter::BeginPause()
{
    // TODO Look at re-entrant lock (from the same thread)
    // rather then home grown
    if (mUsageCounter++)
    {
        return;
    }

    // We should use Concurrency (perf) locker to keep track in RAD
    YLOG_NOTICE("DEVI", "Waiter - Requesting Render pause from Thread: '%s'.", yaget::platform::GetCurrentThreadName().c_str());
    std::unique_lock<std::mutex> locker(mPauseRenderMutex);
    mPauseCounter = true;
    mWaitForRenderThread.wait(locker);
    YLOG_NOTICE("DEVI", "Waiter - Render is Paused (resizing commences...) from Thread: '%s'.", yaget::platform::GetCurrentThreadName().c_str());
}


//-------------------------------------------------------------------------------------------------
void yaget::render::Waiter::EndPause()
{
    if (--mUsageCounter)
    {
        return;
    }

    YLOG_NOTICE("DEVI", "Waiter - Render can start (resizing done) from Thread: '%s'.", yaget::platform::GetCurrentThreadName().c_str());
    mPauseCounter = false;
    mRenderPaused.notify_one();
}
