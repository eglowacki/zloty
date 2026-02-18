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
#include "MathFacade.h"
#include "Time/GameClock.h"

namespace DirectX::SimpleMath   // this is aliased as math3d
{
    using namespace yaget;

    template<typename T>
    struct LerpTrait
    {
        static T lerp(const T& startValue, const T& endValue, float t)
        {
            return std::lerp(startValue, endValue, t);
        }
    };

    template<>
    struct LerpTrait<colors::Color>
    {
        static colors::Color lerp(const colors::Color& startValue, const colors::Color& endValue, float t)
        {
            return colors::Color::Lerp(startValue, endValue, t);
        }
    };


    template<typename T, typename LT = LerpTrait<T>>
    class Interpolator
    {
    public:
        Interpolator(const T& startValue, const T& endValue)
            : mStartValue{ startValue }
            , mEndValue{ endValue }
        {
        }

        T GetValue(const time::GameClock& gameClock)
        {
            T adjustedColor = LT::lerp(mStartValue, mEndValue, mCurrentColorT);
            mCurrentColorT += (gameClock.GetDeltaTimeSecond() * mColorTDirection) * 0.75f;
            if (mCurrentColorT > 1.0f)
            {
                mColorTDirection = -1.0f;
            }
            else if (mCurrentColorT < 0.0f)
            {
                mColorTDirection = 1.0f;
            }

            return adjustedColor;
        }

    private:
        const T mStartValue = {};
        const T mEndValue = {};
        float mCurrentColorT = 0.0f;
        float mColorTDirection = 1.0f;
    };
}
