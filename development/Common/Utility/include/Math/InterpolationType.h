/////////////////////////////////////////////////////////////////////
// InterpolationType.h
//
//  Copyright 12/21/2008 Edgar Glowacki.
//
//  Maintained by: Edgar
//
//  NOTES:
//
//
//  #include "Math/InterpolationType.h"
//
//////////////////////////////////////////////////////////////////////
//! \file

#ifndef MATH_INTERPOLATION_TYPE_H
#define MATH_INTERPOLATION_TYPE_H
#pragma once

#include "Base.h"


namespace eg
{
    namespace math
    {
        /*!
        This class is used to create interpolation types to provide
        diffrent interpolation methods.
        The workhorse of this class is:
            float GetValue(float LowBounds, float HiBounds, float t);

          This method will return value between LowBounds and HiBounds, based on t, where
          if t == 0, then return LowBounds, t == 1, return HiBounds.

          We provide simple interpolation types for common use.
        */
        class InterpolationType
        {
        public:
            InterpolationType() : mReverseT(false), mMirror(false), mMirrorPosition(0.5f)
            {
            }

            InterpolationType(bool reversetT, bool mirror, float mirrorPosition) : mReverseT(reversetT), mMirror(mirror), mMirrorPosition(mirrorPosition)
            {
            }

            /*!
            This will return interpolated value between LowBounds & HiBounds.
            t is a time value between 0 and 1.
            */
            virtual float GetValue(float /*LowBounds*/, float /*HiBounds*/, float /*t*/) const
            {
                return 0.0f;
            }

        protected:
            //! This methods adjust the t value based on the m_Mirror, m_MirrorPosition and m_ReverseT.
            float adjustT(float t) const
            {
                if (mMirror)
                {
                    if (t < mMirrorPosition)
                    {
                        t /= mMirrorPosition;
                    }
                    else
                    {
                        t -= mMirrorPosition;
                        t /= (1.0f - mMirrorPosition);
                        t = 1.0f - t;
                    }
                }
                if (mReverseT)
                    t = 1.0f - t;

                return t;
            }

        private:
            // if this is set to TRUE, then we'll reverse t value, where if t==0, it will become 1 and vice-versa
            bool mReverseT;
            // Mirroring the value will cause the left side of the mirror value to look like the normal interpolation,
            // and the right side to be the reversed interpolation.
            float mMirrorPosition;
            bool mMirror;
        };


        /*!
         * This helper class provides time values scaled from 0 to 1
         * regardless of what delta time is fed to it
         */
        class TimeUnit
        {
        public:
            TimeUnit() : mCurrTime(0), mTimeScale(1) {}
            TimeUnit(float timeScale) : mCurrTime(0), mTimeScale(timeScale) {}

            float Tick(float deltaTime)
            {
                mCurrTime += (deltaTime * mTimeScale);
                clamp();
                return mCurrTime;
            }

        private:
            void clamp()
            {
                if (mCurrTime >= 1.0f)
                {
                    mCurrTime -= 1.0f;
                }
            }

            float mCurrTime;
            float mTimeScale;
        };


        /*!
        Linear interpolation
        HiBounds-LowBounds)*t+LowBounds.
        */
        class LerpType : public InterpolationType
        {
        public:
            LerpType() : InterpolationType() {}
            LerpType(bool reversetT, bool mirror, float mirrorPosition) : InterpolationType(reversetT, mirror, mirrorPosition) {}
            virtual float GetValue(float LowBounds, float HiBounds, float t) const;
        };


        /*!
        Smooth step interpolation
        NormalizeValue = t/1;
        result = (NormalizeValue*NormalizeValue*(3.0-2.0*NormalizeValue));
        */
        class SmoothStepType : public InterpolationType
        {
        public:
            SmoothStepType() : InterpolationType() {}
            SmoothStepType(bool reversetT, bool mirror, float mirrorPosition) : InterpolationType(reversetT, mirror, mirrorPosition) {}
            virtual float GetValue(float LowBounds, float HiBounds, float t) const;
        };


        /*!
        Smooth step interpolation
        result = (coth(t) * sin(t)) / sec(t)
        */
        class CothType : public InterpolationType
        {
        public:
            CothType() : InterpolationType() {}
            CothType(bool reversetT, bool mirror, float mirrorPosition) : InterpolationType(reversetT, mirror, mirrorPosition) {}
            virtual float GetValue(float LowBounds, float HiBounds, float t) const;
        };


        /*!
        This behaves like gamma correction
        by varing gamma value, we can have different curves
        result = pow(t,1/mGamma)
        */
        class GammaType : public InterpolationType
        {
        public:
            GammaType() : InterpolationType(), mGamma(2.3f) {}
            GammaType(float gamma, bool reversetT, bool mirror, float mirrorPosition) : InterpolationType(reversetT, mirror, mirrorPosition), mGamma(gamma) {}
            GammaType(float gamma) : mGamma(gamma) {}

            virtual float GetValue(float LowBounds, float HiBounds, float t) const;

        private:
            float mGamma;
        };


        /*!
        Bias type, similar to Gamma properties
        result = pow(t,log(mBias)/log(0.5))
        where if t = 0.5, then bias(mBias,t) = m_Bias
        */
        class BiasType : public InterpolationType
        {
        public:
            BiasType() : InterpolationType(), mBias(0.2f) {}
            BiasType(float bias, bool reversetT, bool mirror, float mirrorPosition) : InterpolationType(reversetT, mirror, mirrorPosition), mBias(bias) {}
            BiasType(float bias) : mBias(bias) {}

            virtual float GetValue(float LowBounds, float HiBounds, float t) const;

        private:
            float mBias;
        };


        /*!
        Gain type, this extends bias type
        BiasType biasType(1-m_Gain)
        if (t < 0.5)
            result = (biasType.GetValue(LowBounds, HiBounds, 2*t)/2)
        else
            result = 1-(biasType.GetValue(LowBounds, HiBounds, 2-2*t)/2)
        */
        class GainType : public InterpolationType
        {
        public:
            GainType() : InterpolationType(), mGain(0.2f) {}
            GainType(float gain, bool reversetT, bool mirror, float mirrorPosition) : InterpolationType(reversetT, mirror, mirrorPosition), mGain(gain) {}
            GainType(float gain) : mGain(gain) {}

            virtual float GetValue(float LowBounds, float HiBounds, float t) const;

        private:
            float mGain;
        };


        //! quarter circle interpolation
        class QuarterCircleType : public InterpolationType
        {
        public:
            QuarterCircleType() : InterpolationType() {}
            QuarterCircleType(bool reversetT, bool mirror, float mirrorPosition) : InterpolationType(reversetT, mirror, mirrorPosition) {}
            virtual float GetValue(float LowBounds, float HiBounds, float t) const;
        };


        //! sin interpolation
        class SinType : public InterpolationType
        {
        public:
            SinType() : InterpolationType() {}
            SinType(bool reversetT, bool mirror, float mirrorPosition) : InterpolationType(reversetT, mirror, mirrorPosition) {}
            virtual float GetValue(float LowBounds, float HiBounds, float t) const;
        };



    } // namespace math

} // namespace eg

#endif // MATH_INTERPOLATION_TYPE_H


#if 0
#pragma once


/*! \file */

#include <memory>
#include <cassert>











/*
This class handles spline interpolation, based on
Catmull-Rom
class BL_CSplineType : public BL_CInterpolationType
{
    typedef BL_CInterpolationType BASE;
public:
    BL_CSplineType();

    virtual float GetValue(float LowBounds, float HiBounds, float t) const;

private:
    FLOATArray m_Nots;
};
*/

/*!
This is used in conjuction with BL_CInterpolationType classes to provide
simple time interpolation, based on TimePeriod.
*/
class BL_CTimeInterpolator
{
public:
    /*!
    Interpolation is one of the BL_CInterpolationType derived classes created with new operator, which
    this class owns it and will delete it in a dtor.
    */
    BL_CTimeInterpolator(BL_CInterpolationType *Interpolation, float LowValue, float HiValue, float TimePeriod) :
    m_Interpolation(Interpolation), m_LowValue(LowValue), m_HiValue(HiValue), m_TimePeriod(TimePeriod),
    m_LastTime(0)
    {
        assert(m_Interpolation.get());
        m_Interpolation->SetMirror(true);
    }

    //! This will return interpolated value between LowValue and HiValue, based on TimePeriod.
    float Tick(float DeltaTime)
    {
        if (m_LastTime > m_TimePeriod)
            m_LastTime = 0;

        m_LastTime += DeltaTime;
        return m_Interpolation->GetValue(m_LowValue, m_HiValue, m_LastTime/m_TimePeriod);
    }


private:
    float m_LastTime;
    float m_LowValue;
    float m_HiValue;
    float m_TimePeriod;
    std::auto_ptr<BL_CInterpolationType> m_Interpolation;
};


#endif // 0