#include "TimeSourceHRWin32.h"
#include "Platform/WindowsLean.h"


#define USE_UINT
//#define USE_LI
//#define USE_TGT

namespace eg {

TimeSourceHRWin32::TimeSourceHRWin32()
{
#ifdef USE_TGT
    mInitialTime = timeGetTime();
#endif // USE_TGT

#ifdef USE_UINT
    // since we are using QueryPerformanceCounter
    // we need to lock it to single core
    ::SetThreadAffinityMask(::GetCurrentThread(), 1);
    uint64_t pf;
    ::QueryPerformanceFrequency((LARGE_INTEGER *)&pf);
    mFreq = 1.0 / (double)pf;
    QueryPerformanceCounter((LARGE_INTEGER *)&mInitialTime);
#endif // USE_UINT

#ifdef USE_LI
    // since we are using QueryPerformanceCounter
    // we need to lock it to single core
    ::SetThreadAffinityMask(::GetCurrentThread(), 1);
    LARGE_INTEGER freq;
    ::QueryPerformanceFrequency(&freq);
    mSecondsPerTick = 1.0/freq.QuadPart;
#endif // USE_LI
}


double TimeSourceHRWin32::GetTime() const
{
#ifdef USE_TGT
    uint32_t currTime = timeGetTime();
    return (currTime - mInitialTime)/1000.0;
#endif // USE_TGT

#ifdef USE_UINT
    uint64_t val;
    QueryPerformanceCounter((LARGE_INTEGER *)&val);
    return (val - mInitialTime) * mFreq;
    //return val * mFreq;
#endif // USE_UINT

#ifdef USE_LI
    LARGE_INTEGER time;
    ::QueryPerformanceCounter(&time);
    return time.QuadPart * mSecondsPerTick;
#endif // USE_LI
}


} // namespace eg

