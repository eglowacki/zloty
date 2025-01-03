#include "Render/Waiter.h"


//-------------------------------------------------------------------------------------------------
void yaget::render::Waiter::Wait()
{
    if (mPauseCounter == true)
    {
        YLOG_NOTICE("DEVI", "Waiter - We are requested to pause. Stopping.");
        mWaitForRenderThread.notify_one();
        std::unique_lock<std::mutex> locker(mPauseRenderMutex);
        mRenderPaused.wait(locker);
        YLOG_NOTICE("DEVI", "Waiter - Resuming Render.");
    }
}


//-------------------------------------------------------------------------------------------------
void yaget::render::Waiter::BeginPause()
{
    // TODO Look at re-entrant lock (from the same thread)
    // rather then home grown
    if (mUsageCounter++)
    {
        YLOG_ERROR("DEVI", "Waiter BeginPause() called recursevly, why?");
        return;
    }

    // We should use Concurrency (perf) locker to keep track in RAD
    YLOG_NOTICE("DEVI", "Waiter - Requesting Render pause.");
    std::unique_lock<std::mutex> locker(mPauseRenderMutex);
    mPauseCounter = true;
    mWaitForRenderThread.wait(locker);
    YLOG_NOTICE("DEVI", "Waiter - Render is Paused (resizing commences...).");
}


//-------------------------------------------------------------------------------------------------
void yaget::render::Waiter::EndPause()
{
    if (--mUsageCounter)
    {
        YLOG_ERROR("DEVI", "Waiter EndPause() called recursevly, why?");
        return;
    }

    YLOG_NOTICE("DEVI", "Waiter - Render can start (resizing done).");
    mPauseCounter = false;
    mRenderPaused.notify_one();
}
