///////////////////////////////////////////////////////////////////////
// ClockManager.h
//
//  Copyright 12/27/2005 Edgar Glowacki.
//
//  Maintained by: Edgar
//
//  NOTES:
//      Clock Manager which is used to control time and update all the timers.
//
//
//  #include "Timer/ClockManager.h"
//
//////////////////////////////////////////////////////////////////////
//! \file

#ifndef TIMER_CLOCK_MANAGER_H
#define TIMER_CLOCK_MANAGER_H
#pragma once


#include "Base.h"


namespace {class PrivateClockData;}
namespace eg
{
    // forward declarations
    class ITimeSource;

    class IObserver
    {
    public:
        virtual ~IObserver() {}
        virtual void Notify() = 0;
    };

    class ClockManager
    {
        //@{
        //! no copy semantics
        ClockManager(const ClockManager&);
        ClockManager& operator =(const ClockManager&);
        //@}

    public:
        ClockManager(const ITimeSource *pSource = 0);
        ~ClockManager();

        void SetTimeSource(const ITimeSource *pSource);

        void FrameStep();

        double GetTime() const;
        double GetFrameDuration() const;
        int GetFrameNumber() const;
        float GetFrameRate() const;

        void AddObserver(IObserver *pObserver);
        void RemoveObserver(IObserver *pObserver);

        void SetFiltering(int frameWindow, double frameDefault = 0.030);

        //! Return real time continuously running
        double GetRealTime() const;

    private:
        double GetExactLastFrameDuration();
        void AddToFrameHistory(double exactFrameDuration);
        double GetPredictedFrameDuration() const;

        const ITimeSource *mpTimeSource;
        double mCurrentTime;
        double mFrameTime;
        int mFrameNumber;

        double mSourceStartValue;
        double mSourceLastValue;

        int mFrameFilteringWindow;
        double mFrameDefaultTime;

        PrivateClockData *mClockData;
    };

    /*
    To use this as a singleton
    \code
    #include "Registrat.h"
    ClockManager *cm = registrate::p_cast<ClockManager>(wxT("ClockManager"));
    // or ...
    ClockManager& cm = registrate::ref_cast<ClockManager>(wxT("ClockManager"));
    // or ...
    ClockManager& cm = REGISTRATE(ClockManager);   // macro alert
    \endcode
    */

} // namespace eg

#endif // TIMER_CLOCK_MANAGER_H
