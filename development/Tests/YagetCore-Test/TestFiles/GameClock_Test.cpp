#include "pch.h" 
#include "Time/GameClock.h"
#include "MathFacade.h"

#include "Metrics/Concurrency.h"
#include "Metrics/Gather.h"


#include "Platform/Support.h"
#include "TestHelpers/TestHelpers.h"


class Time : public ::testing::Test
{
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

    // check sleep (0.1 second)
    constexpr time::Microsecond_t BusySleepDuration = time::FromTo<int>(2, yaget::time::kMilisecondUnit, yaget::time::kMicrosecondUnit);
    std::map<std::string, time::Microsecond_t> timings{ { "BusySleep", 0 }, { "Sleep", 0 }, { "Sleep-Lambda", 0 } };

    time::Microsecond_t accumulatedError = 0;
    const auto BusySkeepIterations = 10;
    const auto MaximumBusySleepError = static_cast<time::Microsecond_t>(std::ceil((BusySkeepIterations * BusySleepDuration) * 0.01f));  // maximum error for busy sleeps in microseconds
    const auto MaximumSleepError = 0.25f;   // maximum error in regular sleep in %

    for (auto i = 0; i < BusySkeepIterations; ++i)
    {
        const time::Microsecond_t stampStart = platform::GetRealTime(time::kMicrosecondUnit);
        platform::BusySleep(BusySleepDuration, time::kMicrosecondUnit);
        const time::Microsecond_t stampEnd = platform::GetRealTime(time::kMicrosecondUnit);

        const time::Microsecond_t diff = stampEnd - stampStart;
        accumulatedError += std::abs(BusySleepDuration - diff);
    }

    // the error should not be bigger then MaximumBusySleepError ms
    EXPECT_LE(accumulatedError / BusySkeepIterations, MaximumBusySleepError);
    timings["BusySleep"] = accumulatedError / BusySkeepIterations;
    accumulatedError = 0;

    const auto SleepIteration = 4;
    const auto SleepDuration = time::FromTo<int>(16, yaget::time::kMilisecondUnit, yaget::time::kMicrosecondUnit);
    //-------------------------------
    for (auto i = 0; i < SleepIteration; ++i)
    {
        // check sleep (0.1 second) using platform dependent sleep functionality
        const time::Microsecond_t stampStart = platform::GetRealTime(time::kMicrosecondUnit);
        platform::Sleep(SleepDuration, time::kMicrosecondUnit);
        const time::Microsecond_t stampEnd = platform::GetRealTime(time::kMicrosecondUnit);

        const time::Microsecond_t diff = stampEnd - stampStart;
        accumulatedError += std::abs(SleepDuration - diff);
    }
    // the error should not be bigger then MaximumSleepError percentage of total sleep time
    EXPECT_LE(accumulatedError / SleepIteration, (SleepIteration * SleepDuration) * MaximumSleepError);
    timings["Sleep"] = accumulatedError / SleepIteration;
    accumulatedError = 0;

    //-------------------------------
    for (auto i = 0; i < BusySkeepIterations; ++i)
    {
        const time::Microsecond_t stampStart = platform::GetRealTime(time::kMicrosecondUnit);
        time::Microsecond_t endSleepTime = stampStart + BusySleepDuration;
        platform::Sleep([endSleepTime]()
            {
                return platform::GetRealTime(time::kMicrosecondUnit) < endSleepTime;
            });

        const time::Microsecond_t stampEnd = platform::GetRealTime(time::kMicrosecondUnit);

        const time::Microsecond_t diff = stampEnd - stampStart;
        accumulatedError += std::abs(BusySleepDuration - diff);
    }

    // the error should not be bigger then MaximumBusySleepError
    EXPECT_LE(accumulatedError / BusySkeepIterations, MaximumBusySleepError);
    timings["Sleep-Lambda"] = accumulatedError / BusySkeepIterations;
    accumulatedError = 0;

    //-------------------------------
    std::string loadsMessage = fmt::format("Accumulated errors after '{}' iterations:", BusySkeepIterations);
    for (const auto& elem : timings)
    {
        loadsMessage += fmt::format("\n\t{} = {} {}.", elem.first, conv::ToThousandsSep(elem.second), metrics::UnitName(time::kMicrosecondUnit));
    }

    loadsMessage += fmt::format("\n\tBusySleep: {} {}, Sleep: {} {}.", 
                                    conv::ToThousandsSep(BusySleepDuration), metrics::UnitName(time::kMicrosecondUnit),
                                    conv::ToThousandsSep(SleepDuration), metrics::UnitName(time::kMicrosecondUnit));

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

namespace
{
    std::tuple<double, double> SetupTimeDuration(yaget::time::TimeUnits_t waitTime, yaget::time::TimeUnits_t unitType, std::function<void(yaget::time::TimeUnits_t maxSleepSleep, yaget::time::TimeUnits_t unitType)> sleeperFunction)
    {
        using namespace yaget;

        const auto start = std::chrono::system_clock::now();

        sleeperFunction(waitTime, unitType);

        const auto end = std::chrono::system_clock::now();
        const std::chrono::duration<double> diff = end - start;
        const auto timePoint = diff.count();

        double expectedTime = time::FromTo<double>(waitTime, unitType, time::kSecondUnit);

        return { timePoint, expectedTime };
    }
    
}

TEST_F(Time, ClockDuration)
{
    using namespace yaget;

    {
        constexpr time::TimeUnits_t waitTime = 500;
        constexpr time::TimeUnits_t unitType = time::kMilisecondUnit;

        const auto [timePoint, expectedTime] = SetupTimeDuration(waitTime, unitType, [](auto waitTime, auto unitType)
        {
            platform::Sleep(waitTime, unitType);
        });

        EXPECT_NEAR(expectedTime, timePoint, 0.05);
    }

    {
        constexpr time::TimeUnits_t waitTime = 16;
        constexpr time::TimeUnits_t unitType = time::kMilisecondUnit;

        const auto [timePoint, expectedTime] = SetupTimeDuration(waitTime, unitType, [](auto waitTime, auto unitType)
        {
            platform::Sleep(waitTime, unitType);
        });

        EXPECT_NEAR(expectedTime, timePoint, 0.05);
    }

    {
        constexpr time::TimeUnits_t waitTime = time::FromTo<time::Microsecond_t>(500, time::kMilisecondUnit, time::kMicrosecondUnit);
        constexpr time::TimeUnits_t unitType = time::kMicrosecondUnit;

        const auto [timePoint, expectedTime] = SetupTimeDuration(waitTime, unitType, [](auto waitTime, auto unitType)
        {
            platform::Sleep(waitTime, unitType);
        });

        EXPECT_NEAR(expectedTime, timePoint, 0.05);
    }

    {
        constexpr time::TimeUnits_t waitTime = 500;
        constexpr time::TimeUnits_t unitType = time::kMilisecondUnit;

        const auto [timePoint, expectedTime] = SetupTimeDuration(waitTime, unitType, [](auto waitTime, auto unitType)
        {
            platform::BusySleep(waitTime, unitType);
        });

        EXPECT_NEAR(expectedTime, timePoint, 0.05);
    }

    {
        constexpr time::TimeUnits_t waitTime = time::FromTo<time::Microsecond_t>(500, time::kMilisecondUnit, time::kMicrosecondUnit);
        constexpr time::TimeUnits_t unitType = time::kMicrosecondUnit;

        const auto [timePoint, expectedTime] = SetupTimeDuration(waitTime, unitType, [](auto waitTime, auto unitType)
        {
            platform::BusySleep(waitTime, unitType);
        });

        EXPECT_NEAR(expectedTime, timePoint, 0.05);
    }
}
