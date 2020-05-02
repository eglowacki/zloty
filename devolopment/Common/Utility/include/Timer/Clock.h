///////////////////////////////////////////////////////////////////////
// Clock.h
//
//  Copyright 12/27/2005 Edgar Glowacki.
//
//  Maintained by: Edgar
//
//  NOTES:
//      Timer which is updated from ClockManager
//
//
//  #include "Timer/Clock.h"
//
//////////////////////////////////////////////////////////////////////
//! \file

#ifndef TIMER_CLOCK_H
#define TIMER_CLOCK_H
#pragma once


#include "ClockManager.h"


namespace eg
{
    class ClockManager;

    /*!
    Simple Timer which allows us to pause and scale time
    */
    class Clock : private IObserver
    {
        //! no copy semantics
        Clock(const Clock&);
        Clock& operator=(const Clock&);

    public:
        Clock(ClockManager& clock);
        virtual ~Clock();

        double GetTime() const;
        float GetFrameDuration() const;
        float GetFrameRate() const;

        void Pause(bool bOn);
        void SetScale(float fScale);

        bool IsPaused() const;
        float GetScale() const;

    private:
        // from IObserver
        virtual void Notify();

        ClockManager& mClock;

        double mCurrentTime;
        double mFrameTime;

        bool mbPaused;
        float mScale;
    };

    //---------------------------------------------------------------------------
    inline Clock::Clock(ClockManager& clock) :
        mClock(clock),
        mCurrentTime(0),
        mFrameTime(0),
        mbPaused(false),
        mScale(1.0f)
    {
        mClock.AddObserver(this);
    }

    //---------------------------------------------------------------------------
    inline Clock::~Clock()
    {
        mClock.RemoveObserver(this);
    }

    //---------------------------------------------------------------------------
    inline double Clock::GetTime() const
    {
        return mCurrentTime;
    }

    //---------------------------------------------------------------------------
    inline float Clock::GetFrameDuration() const
    {
        return static_cast<float>(mFrameTime);
    }

    inline float Clock::GetFrameRate() const
    {
        return mClock.GetFrameRate();
    }

    //---------------------------------------------------------------------------
    inline void Clock::Pause(bool bOn)
    {
        mbPaused = bOn;
    }

    //---------------------------------------------------------------------------
    inline void Clock::SetScale(float fScale)
    {
        mScale = fScale;
    }

    //---------------------------------------------------------------------------
    inline bool Clock::IsPaused() const
    {
        return mbPaused;
    }

    //---------------------------------------------------------------------------
    inline float Clock::GetScale() const
    {
        return mScale;
    }

    inline void Clock::Notify()
    {
        if (!mbPaused)
        {
            mFrameTime = mClock.GetFrameDuration() * mScale;
            mCurrentTime += mFrameTime;
        }
    }


} // namespace eg


#endif // TIMER_CLOCK_H
