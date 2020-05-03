//////////////////////////////////////////////////////////////////////
// ClockUtilities.h
//
//  Copyright 12/1/2007 Edgar Glowacki.
//
//  Maintained by: Edgar
//
//  NOTES:
//      Simple timer class which allows us to be notified
//      at specified frequency
//
//
//  #include "Timer/ClockUtilities.h"
//
//////////////////////////////////////////////////////////////////////
//! \file

#ifndef _TIMER_CLOCK_UTILITIES_H
#define _TIMER_CLOCK_UTILITIES_H
#pragma once

#include "Message/Dispatcher.h"
#include <boost/function.hpp>
#include <boost/signals.hpp>


namespace eg
{
	// Helpful constants for timer usage
	namespace timer
	{
		//! This is minimum allowed frequency which translates to call once every 10 minutes
		const float kMinFreq = 0.0001f;
		//! This is maximum allowed frequency which translates to call once every 1 milisecond
		const float kMaxFreq = 1000.0f;

		inline float clampFreq(float freq) {return std::min(kMaxFreq, std::max(kMinFreq, freq));}
	} // namespace timer

	/*!
	* This timer will trigger callback at specified frequency (times per second)
	*
	*/
	class IntervalTimer : public boost::signals::trackable
    {
    public:
        typedef boost::function<void (Message& /*msg*/)> IntervalTimerCallback_t;

        IntervalTimer(IntervalTimerCallback_t callback, float frequency)
		: mElapsedTime(0)
        , mCallback(callback)
		, mInterval(1.0f/timer::clampFreq(frequency))
        {
            Dispatcher& disp = REGISTRATE(Dispatcher);
            disp[mtype::kFrameTick].connect(boost::ref(*this));
        }

        //! Allows user to change frequency of callbacks.
        //! This does not preserve old frequency value.
        void SetFrequency(float frequency) {mInterval = 1.0f/timer::clampFreq(frequency);}

        //! Called by Dispatcher on kFrameTick event
        void operator()(Message& msg)
        {
            if (mCallback)
            {
                mElapsedTime += msg.GetValue<float>();
                if (mElapsedTime >= mInterval)
                {
                    float elapsed = mElapsedTime;
					mElapsedTime = getElapsedRemain(mElapsedTime, mInterval);

                    eg::Message newMsg(msg.mType, elapsed);
                    mCallback(newMsg);
                }
            }
        }

    private:
        float mElapsedTime;
        float mInterval;
        IntervalTimerCallback_t mCallback;

		float getElapsedRemain(float elapsedTime, float interval) const
		{
			int times = elapsedTime/interval;
			float currTotal = times * interval;
			float remain = elapsedTime - currTotal;
			return remain;
		}
    };

} // namespace eg

#endif // _TIMER_CLOCK_UTILITIES_H

