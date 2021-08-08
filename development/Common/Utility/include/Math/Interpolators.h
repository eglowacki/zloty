/////////////////////////////////////////////////////////////////////////
// Interpolators.h
//
//  Copyright July 17, 2021 Edgar Glowacki.
//
// NOTES:
//      
//
// #include "Math/Interpolators.h"
//
/////////////////////////////////////////////////////////////////////////
//! \file

#pragma once

#include "Math/YagetMath.h"

namespace yaget::math
{
    enum class InterpolatorDirection { Up, Down, Both };

    template<typename T, InterpolatorDirection D = InterpolatorDirection::Up>
    struct Interpolator
    {
        Interpolator(T startValue, T endValue)
            : mStartValue{ startValue }
            , mEndValue{ endValue }
            , mCurrentValue{ mStartValue }
        {}

        const T& Update(float dt)
        {
            mCurrentT += dt * mDirection;

            if (D == InterpolatorDirection::Both)
            {
                if (mDirection > 0.0f && mCurrentT > 1.0f)
                {
                    mDirection = -1.0f;
                    mCurrentT = 1.0f - (mCurrentT - 1.0f);
                }
                else if (mDirection < 0.0f && mCurrentT < 0.0f)
                {
                    mDirection = 1.0f;
                    mCurrentT = mCurrentT * -1.0f;
                }
            }
            mCurrentValue = std::lerp(mStartValue, mEndValue, mCurrentT);

            if (D != InterpolatorDirection::Both)
            {
                if (mStartValue <= mEndValue)
                {
                    mCurrentValue = std::clamp(mCurrentValue, mStartValue, mEndValue);
                }
                else
                {
                    mCurrentValue = std::clamp(mCurrentValue, mEndValue, mStartValue);
                }
            }

            return mCurrentValue;
        }

        const T& Value() const { return mCurrentValue; }

    private:
        const T mStartValue;
        const T mEndValue;
        T mCurrentValue;
        float mCurrentT = D == InterpolatorDirection::Down ? 1.0f : 0.0f;
        float mDirection = D != InterpolatorDirection::Both ? (D == InterpolatorDirection::Up ? 1.0f : -1.0f) : 1.0f;
    };

} // namespace yaget::math

