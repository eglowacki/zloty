#include "../Win32/TimeSourceHRWin32.h"
#include "Profiler/prof_gather.h"
#define WIN32_LEAN_AND_MEAN
#define WIN32_EXTRA_LEAN
#include <windows.h>
#include <assert.h>


Prof_extern_C double Prof_get_time(void)
{
    static eg::TimeSourceHRWin32 timer;
    return timer.GetTime();
    /*
    LARGE_INTEGER freq;
    LARGE_INTEGER time;

    BOOL ok = QueryPerformanceFrequency(&freq);
    assert(ok == TRUE);

    freq.QuadPart = freq.QuadPart;

    ok = QueryPerformanceCounter(&time);
    assert(ok == TRUE);

    return time.QuadPart / (double) freq.QuadPart;
    */
}
