#include "Metrics/Concurrency.h"

#include "Render/Waiter.h"


//-------------------------------------------------------------------------------------------------
void yaget::render::Waiter::Wait()
{
    if (mPauseCounter == true)
    {
        const char* message = "Waiter - We are requested to pause. Stopping.";
        metrics::Channel systemChannel(message);
        YLOG_INFO("DEVI", message);
        mWaitForRenderThread.notify_one();
        std::unique_lock<std::mutex> locker(mPauseRenderMutex);
        mRenderPaused.wait(locker);
        YLOG_INFO("DEVI", "Waiter - Resuming Render.");
    }
}


//-------------------------------------------------------------------------------------------------
void yaget::render::Waiter::BeginPause()
{
    // TODO(eg) Look at re-entrant lock (from the same thread)
    // rather than home-grown
    if (mUsageCounter++)
    {
        YLOG_ERROR("DEVI", "Waiter BeginPause() called recursively, why?");
        return;
    }

    const char* message = "Waiter - Requesting Render pause.";
    metrics::Channel systemChannel(message);
    // We should use Concurrency (perf) locker to keep track in RAD
    YLOG_INFO("DEVI", message);
    std::unique_lock<std::mutex> locker(mPauseRenderMutex);
    mPauseCounter = true;
    mWaitForRenderThread.wait(locker);
    YLOG_INFO("DEVI", "Waiter - Render is Paused (resizing commences...).");
}


//-------------------------------------------------------------------------------------------------
void yaget::render::Waiter::EndPause()
{
    if (--mUsageCounter)
    {
        YLOG_ERROR("DEVI", "Waiter EndPause() called recursively, why?");
        return;
    }

    const char* message = "Waiter - Render can start (resizing done).";
    metrics::Channel systemChannel(message);
    YLOG_INFO("DEVI", message);
    mPauseCounter = false;
    mRenderPaused.notify_one();
}
