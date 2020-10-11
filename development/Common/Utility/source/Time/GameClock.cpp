#include "Time/GameClock.h"
#include "Platform/Support.h"


yaget::time::GameClock::GameClock()
{
    Resync();
}

yaget::time::Microsecond_t yaget::time::GameClock::GetLogicTime() const
{
    return mLogicTime;
}

yaget::time::Microsecond_t yaget::time::GameClock::GetDeltaTime() const
{
    return mDeltaTime;
}

void yaget::time::GameClock::Resync()
{
    mResetTime = platform::GetRealTime(yaget::time::kMicrosecondUnit);
    mLogicTime = mResetTime.load();
    mDeltaTime = 0;
    mTickCounter = 0;
}

void yaget::time::GameClock::Tick(yaget::time::Microsecond_t deltaTime)
{
    mLogicTime += deltaTime;
    mDeltaTime = deltaTime;
    mTickCounter++;
}

yaget::time::Microsecond_t yaget::time::GameClock::GetRealTime() const
{
    return platform::GetRealTime(yaget::time::kMicrosecondUnit);
}