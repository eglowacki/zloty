#include "pch.h" 
#include "TestHelpers/TestHelpers.h"
#include "Math/Interpolators.h"


//CHECK_EQUAL(expected, actual);

class Math : public ::testing::Test
{
};

TEST_F(Math, InterpolatorUp)
{
    using namespace yaget;

    const float startValue = 0.0f;
    const float endValue = 12.0f;
    const float stepT = 0.5;

    math::Interpolator<float> interpolator(startValue, endValue);
    EXPECT_FLOAT_EQ(startValue, interpolator.Value());

    interpolator.Update(stepT);
    EXPECT_FLOAT_EQ(6.0f, interpolator.Value());

    interpolator.Update(stepT);
    EXPECT_FLOAT_EQ(endValue, interpolator.Value());

    interpolator.Update(stepT);
    EXPECT_FLOAT_EQ(endValue, interpolator.Value());
}


TEST_F(Math, InterpolatorDown)
{
    using namespace yaget;

    const float startValue = 12.0f;
    const float endValue = 0.0f;
    const float stepT = 0.5;

    math::Interpolator<float> interpolator(startValue, endValue);
    EXPECT_FLOAT_EQ(startValue, interpolator.Value());

    interpolator.Update(stepT);
    EXPECT_FLOAT_EQ(6.0f, interpolator.Value());

    interpolator.Update(stepT);
    EXPECT_FLOAT_EQ(endValue, interpolator.Value());

    interpolator.Update(stepT);
    EXPECT_FLOAT_EQ(endValue, interpolator.Value());
}
