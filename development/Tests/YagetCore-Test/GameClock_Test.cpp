#include "pch.h" 
#include "Time/GameClock.h"
#include "MathFacade.h"

#include "Metrics/Concurrency.h"
#include "Metrics/Gather.h"


#include "Platform/Support.h"
#include "TestHelpers/TestHelpers.h"


class Time : public ::testing::Test
{
protected:
    // void SetUp() override {}
    // void TearDown() override {}

private:
    yaget::test::Environment mEnvironment;
};


TEST_F(Time, GameClock)
{
    yaget::time::GameClock gameClock;
    gameClock.Resync();
    const uint64_t startTime = gameClock.GetLogicTime();

    const int kIterations = 5;
    const uint64_t kStep = 25;
    for (int i = 0; i < kIterations; ++i)
    {
        gameClock.Tick(kStep);
    }

    const uint64_t endTime = gameClock.GetLogicTime();
    const uint64_t dif = endTime - startTime;
    EXPECT_EQ(dif, kIterations * kStep);
}

TEST_F(Time, Sleep)
{
    using namespace yaget;

    // check busy sleep (0.1 second)
    constexpr time::Microsecond_t sleepDuration = time::FromTo<int>(100, yaget::time::kMilisecondUnit, yaget::time::kMicrosecondUnit);
    std::map<std::string, time::Microsecond_t> timings{ { "BusySleep", 0 }, { "Sleep", 0 }, { "Sleep-Lambda", 0 } };

    time::Microsecond_t accumulatedError = 0;
    const auto Iterations = 5;
    for (auto i = 0; i < Iterations; ++i)
    {
        const time::Microsecond_t stampStart = platform::GetRealTime(time::kMicrosecondUnit);
        platform::BusySleep(sleepDuration, time::kMicrosecondUnit);
        const time::Microsecond_t stampEnd = platform::GetRealTime(time::kMicrosecondUnit);

        const time::Microsecond_t diff = stampEnd - stampStart;
        accumulatedError += std::abs(sleepDuration - diff);
    }

    // the error should not be bigger then 1 ms
    EXPECT_LE(accumulatedError / Iterations, 1000);
    timings["BusySleep"] = accumulatedError / Iterations;
    accumulatedError = 0;

    for (auto i = 0; i < Iterations; ++i)
    {
        // check sleep (0.1 second) using platform dependent sleep functionality
        const time::Microsecond_t stampStart = platform::GetRealTime(time::kMicrosecondUnit);
        platform::Sleep(sleepDuration, time::kMicrosecondUnit);
        const time::Microsecond_t stampEnd = platform::GetRealTime(time::kMicrosecondUnit);

        const time::Microsecond_t diff = stampEnd - stampStart;
        accumulatedError += std::abs(sleepDuration - diff);
    }

    // the error should not be bigger then 2% of total sleep time
    EXPECT_LE(accumulatedError / Iterations, (Iterations * sleepDuration) * 0.02);
    timings["Sleep"] = accumulatedError / Iterations;
    accumulatedError = 0;

    for (auto i = 0; i < Iterations; ++i)
    {
        const time::Microsecond_t stampStart = platform::GetRealTime(time::kMicrosecondUnit);
        time::Microsecond_t endSleepTime = stampStart + sleepDuration;
        platform::Sleep([endSleepTime]()
            {
                return platform::GetRealTime(time::kMicrosecondUnit) < endSleepTime;
            });

        const time::Microsecond_t stampEnd = platform::GetRealTime(time::kMicrosecondUnit);

        const time::Microsecond_t diff = stampEnd - stampStart;
        accumulatedError += std::abs(sleepDuration - diff);
    }

    // the error should not be bigger then 1 ms
    EXPECT_LE(accumulatedError / Iterations, 1000);
    timings["Sleep-Lambda"] = accumulatedError / Iterations;
    accumulatedError = 0;

    std::string loadsMessage = fmt::format("Accumulated errors after '{}' iterations:", Iterations);
    for (const auto& elem : timings)
    {
        loadsMessage += fmt::format("\n\t{} = {} {}.", elem.first, elem.second, metrics::UnitName(time::kMicrosecondUnit));
    }

    YLOG_NOTICE("TEST", loadsMessage.c_str());
}

TEST_F(Time, FromToConversion)
{
    using namespace yaget;

    const time::TimeUnits_t expectedMicrosecondsTime = 500000;    // this is 500 millisecond, 0.5 seconds
    const time::TimeUnits_t expectedMillisecondsTime = 500;
    const float expectedSecondsTime = 0.5f;

    time::TimeUnits_t resultMillisecondsTime = time::FromTo<time::TimeUnits_t>(expectedMicrosecondsTime, time::kMicrosecondUnit, time::kMilisecondUnit);
    EXPECT_EQ(expectedMillisecondsTime, resultMillisecondsTime);

    float resultSecondsTime = time::FromTo<float>(expectedMicrosecondsTime, time::kMicrosecondUnit, time::kSecondUnit);
    EXPECT_EQ(expectedSecondsTime, resultSecondsTime);

    resultSecondsTime = time::FromTo<float>(resultMillisecondsTime, time::kMilisecondUnit, time::kSecondUnit);
    EXPECT_EQ(expectedSecondsTime, resultSecondsTime);

    resultMillisecondsTime = time::FromTo<time::TimeUnits_t>(resultSecondsTime, time::kSecondUnit, time::kMilisecondUnit);
    EXPECT_EQ(expectedMillisecondsTime, resultMillisecondsTime);

    time::TimeUnits_t resultMicrosecondsTime = time::FromTo<time::TimeUnits_t>(resultSecondsTime, time::kSecondUnit, time::kMicrosecondUnit);
    EXPECT_EQ(expectedMicrosecondsTime, resultMicrosecondsTime);

    resultMicrosecondsTime = time::FromTo<time::TimeUnits_t>(resultMillisecondsTime, time::kMilisecondUnit, time::kMicrosecondUnit);
    EXPECT_EQ(expectedMicrosecondsTime, resultMicrosecondsTime);
}
