// implementation file for ValueInterFLOAT
#include "Math/InterpolationType.h"
#include "Math/Vector.h"


namespace eg {
namespace math {

float LerpType::GetValue(float LowBounds, float HiBounds, float t) const
{
    t = adjustT(t);
    Clamp(t, 0.0f, 1.0f);
    float returnValue = (HiBounds - LowBounds) * t + LowBounds;
    Clamp(returnValue, LowBounds, HiBounds);
    return returnValue;
}


float SmoothStepType::GetValue(float LowBounds, float HiBounds, float t) const
{
    t = adjustT(t);
    Clamp(t, 0.0f, 1.0f);
    float normalizeValue = t / 1.0f;
    float result = (normalizeValue * normalizeValue * (3.0f - 2.0f * normalizeValue));

    LerpType lerp;
    float returnValue = lerp.GetValue(LowBounds, HiBounds, result);
    Clamp(returnValue, LowBounds, HiBounds);
    return returnValue;
}


float CothType::GetValue(float LowBounds, float HiBounds, float t) const
{
    t = adjustT(t);
    Clamp(t, 0.0f, 1.0f);
    LerpType lerp;
    float x = lerp.GetValue(-1.0f, 1.0f, t);
    double x2 = x * x;
    double result = 1 - x2 * (0.3333333 + x2 * (0.1111111 - x2 * (0.0486773 - 0.0081129 * x2)));

    float returnValue = lerp.GetValue(LowBounds, HiBounds, (float)result);
    Clamp(returnValue, LowBounds, HiBounds);
    return returnValue;
}


float GammaType::GetValue(float LowBounds, float HiBounds, float t) const
{
    t = adjustT(t);
    Clamp(t, 0.0f, 1.0f);
    float result = Pow(t, 1.0f / mGamma);

    LerpType lerp;
    float returnValue = lerp.GetValue(LowBounds, HiBounds, (float)result);
    Clamp(returnValue, LowBounds, HiBounds);
    return returnValue;
}


float BiasType::GetValue(float LowBounds, float HiBounds, float t) const
{
    t = adjustT(t);
    Clamp(t, 0.0f, 1.0f);
    float result = Pow(t, Log(mBias) / Log(0.5f));

    LerpType lerp;
    float returnValue = lerp.GetValue(LowBounds, HiBounds, (float)result);
    Clamp(returnValue, LowBounds, HiBounds);
    return returnValue;
}


float GainType::GetValue(float LowBounds, float HiBounds, float t) const
{
    t = adjustT(t);
    Clamp(t, 0.0f, 1.0f);
    BiasType biasType(1.0f - mGain);
    float result;

    if (t < 0.5f)
        result = biasType.GetValue(LowBounds, HiBounds, 2.0f * t) / 2.0f;
    else
        result = 1.0f - (biasType.GetValue(LowBounds, HiBounds, 2.0f - 2.0f * t) / 2.0f);

    LerpType lerp;
    float returnValue = lerp.GetValue(LowBounds, HiBounds, (float)result);
    Clamp(returnValue, LowBounds, HiBounds);
    return returnValue;
}


float QuarterCircleType::GetValue(float LowBounds, float HiBounds, float t) const
{
    t = adjustT(t);
    Clamp(t, 0.0f, 1.0f);

    float result = Sin(Acos(t));
    LerpType lerp;
    float returnValue = lerp.GetValue(LowBounds, HiBounds, (float)result);
    Clamp(returnValue, LowBounds, HiBounds);
    return returnValue;
}


float SinType::GetValue(float LowBounds, float HiBounds, float t) const
{
    t = adjustT(t);
    Clamp(t, 0.0f, 1.0f);

    float result = Sin(DegToRad(t * 90.0f));
    LerpType lerp;
    float returnValue = lerp.GetValue(LowBounds, HiBounds, (float)result);
    Clamp(returnValue, LowBounds, HiBounds);
    return returnValue;
}

} // namespace math
} // namespace eg



#if 0
#include <Math\mtxlib.h>





/**
// spline based interpolation type
BL_CSplineType::BL_CSplineType()
{
    m_Nots+=0.0f;
    m_Nots+=0.9f;
    m_Nots+=0.7f;
    m_Nots+=0.5f;
    m_Nots+=0.2f;
    m_Nots+=0.4f;
    m_Nots+=0.0f;
}

#define CR00    -0.5f
#define CR01     1.5f
#define CR02    -1.5f
#define CR03     0.5f
#define CR10     1.0f
#define CR11    -2.5f
#define CR12     2.0f
#define CR13    -0.5f
#define CR20    -0.5f
#define CR21     0.0f
#define CR22     0.5f
#define CR23     0.0f
#define CR30     0.0f
#define CR31     1.0f
#define CR32     0.0f
#define CR33     0.0f

float BL_CSplineType::GetValue(float LowBounds, float HiBounds, float t) const
{
    t=AdjustT(t);
    gClamp(t,0.0f,1.0f);
    FLOATArray Nots;
    Nots=m_Nots;
    INT span;
    INT nspans=Nots.Num()-3;
    float c0,c1,c2,c3;
    // we need to have at least 4 knots
    if ( nspans<1 )
        return 0.0f;

    t*=nspans;
    span=(INT)t;
    if ( span>=Nots.Num()-3 )
        span=Nots.Num()-3;

    t-=span;
    Nots[0]+=span;

    c3=CR00*Nots[0]+CR01*Nots[1]+CR02*Nots[2]+CR03*Nots[3];
    c2=CR10*Nots[0]+CR11*Nots[1]+CR12*Nots[2]+CR13*Nots[3];
    c1=CR20*Nots[0]+CR21*Nots[1]+CR22*Nots[2]+CR23*Nots[3];
    c0=CR30*Nots[0]+CR31*Nots[1]+CR32*Nots[2]+CR33*Nots[3];

    float result=((c3*t+c2)*t+c1)*t+c0;
    BL_CLerpType Lerp;
    float ReturnValue=Lerp.GetValue(LowBounds,HiBounds,(float)result);
    return ReturnValue;
}
**/


#endif // 0