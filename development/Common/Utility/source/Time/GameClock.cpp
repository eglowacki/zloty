#include "Time/GameClock.h"
#include "Platform/Support.h"


//-------------------------------------------------------------------------------------------------
yaget::time::GameClock::GameClock()
{
    Resync();
}


//-------------------------------------------------------------------------------------------------
yaget::time::Microsecond_t yaget::time::GameClock::GetLogicTime() const
{
    return mLogicTime;
}


//-------------------------------------------------------------------------------------------------
yaget::time::Microsecond_t yaget::time::GameClock::GetDeltaTime() const
{
    return mDeltaTime;
}


//-------------------------------------------------------------------------------------------------
float yaget::time::GameClock::GetDeltaTimeSecond() const
{
    const auto dt = GetDeltaTime();
    const float deltaTime = time::FromTo<float>(dt, time::kMicrosecondUnit, time::kSecondUnit);

    return deltaTime;
}


//-------------------------------------------------------------------------------------------------
void yaget::time::GameClock::Resync()
{
    mResetTime = platform::GetRealTime(yaget::time::kMicrosecondUnit);
    mLogicTime = mResetTime.load();
    mDeltaTime = 0;
    mTickCounter = 0;
}


//-------------------------------------------------------------------------------------------------
void yaget::time::GameClock::Tick(yaget::time::Microsecond_t deltaTime)
{
    mLogicTime += deltaTime;
    mDeltaTime = deltaTime;
    ++mTickCounter;
}


//-------------------------------------------------------------------------------------------------
yaget::time::Microsecond_t yaget::time::GameClock::GetRealTime() const
{
    return platform::GetRealTime(yaget::time::kMicrosecondUnit);
}
