#include "Platform/Support.h"
#include "Time/DeltaClock.h"


yaget::time::DeltaClock::DeltaClock(double deltaTimeFrequency)
    : mDeltaTimeFrequency(deltaTimeFrequency)
    , mLastUpdateTime(platform::GetRealTime())
{
}   


bool yaget::time::DeltaClock::IsDeltaTimePassed()
{
    auto currentTime = platform::GetRealTime();
    auto timeDelta = currentTime - mLastUpdateTime;
    mLastUpdateTime = currentTime;
    mDeltaLeftOver += timeDelta;
    if (mDeltaLeftOver > mDeltaTimeFrequency)
    {
        mDeltaLeftOver = 0;
        return true;
    }

    return false;
}
