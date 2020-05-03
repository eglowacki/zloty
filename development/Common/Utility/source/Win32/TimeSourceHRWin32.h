///////////////////////////////////////////////////////////////////////
// TimeSourceHRWin32.h
//
//  Copyright 12/27/2005 Edgar Glowacki.
//
//  Maintained by: Edgar
//
//  NOTES:
//      High Resolution Time source for Win32 platform
//      \note this should not be exposed like this
//
//
//  #include "Win32/TimeSourceHRWin32.h"
//
//////////////////////////////////////////////////////////////////////
//! \file

#ifndef TIMER_TIME_SOURCE_HR_WIN32_H
#define TIMER_TIME_SOURCE_HR_WIN32_H
#pragma once


#include "Timer/ITimeSource.h"


namespace eg
{

    class TimeSourceHRWin32 : public ITimeSource
    {
    public:
        TimeSourceHRWin32();
        virtual double GetTime() const;

    private:
        double mSecondsPerTick;
        //uint32_t mInitialTime;
        double mFreq;
        uint64_t mInitialTime;

    };


} // namespace eg

#endif // TIMER_TIME_SOURCE_HR_WIN32_H
