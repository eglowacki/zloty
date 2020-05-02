// GameClockUnitTest.cpp : unit test game clock
//

#include "YagetCore.h"
#include "Time/GameClock.h"
#include "Platform/Support.h"
#include "UnitTest++.h"


TEST(GameClock)
{
    yaget::time::GameClock gameClock;
    gameClock.Resync();
    uint64_t startTime = gameClock.GetLogicTime();

    const int kIterations = 5;
    const uint64_t kStep = 25;
    for (int i = 0; i < kIterations; ++i)
    {
        gameClock.Tick(kStep);
    }

    uint64_t endTime = gameClock.GetLogicTime();
    uint64_t dif = endTime - startTime;
    CHECK_EQUAL(dif, kIterations * kStep);
}

TEST(Sleep)
{
    using namespace yaget;

    // check busy sleep (0.1 second)
    constexpr time::Microsecond_t sleepDuration = time::FromTo<int>(100, yaget::time::kMilisecondUnit, yaget::time::kMicrosecondUnit);

    time::Microsecond_t stampStart = platform::GetRealTime(time::kMicrosecondUnit);
    platform::BusySleep(sleepDuration, time::kMicrosecondUnit);
    time::Microsecond_t stampEnd = platform::GetRealTime(time::kMicrosecondUnit);

    time::Microsecond_t diff = stampEnd - stampStart;
    CHECK_CLOSE(diff, sleepDuration, 1000);

    // check sleep (0.1 second) using platform dependent sleep functionality
    stampStart = yaget::platform::GetRealTime(time::kMicrosecondUnit);
    yaget::platform::Sleep(sleepDuration, time::kMicrosecondUnit);
    stampEnd = platform::GetRealTime(time::kMicrosecondUnit);

    diff = stampEnd - stampStart;
    CHECK_CLOSE(sleepDuration, diff, 1000);

    stampStart = platform::GetRealTime(time::kMicrosecondUnit);
    time::Microsecond_t endSleepTime = stampStart + sleepDuration;
    yaget::platform::Sleep([endSleepTime]()
    {
        return platform::GetRealTime(time::kMicrosecondUnit) < endSleepTime;
    });

    stampEnd = yaget::platform::GetRealTime(time::kMicrosecondUnit);
    diff = stampEnd - stampStart;
    CHECK_CLOSE(sleepDuration, diff, 1000);
}

TEST(FromToTimeConversion)
{
    using namespace yaget;

    const time::TimeUnits_t expectedMicrosecondsTime = 500000;    // this is 500 millisecond, 0.5 seconds
    const time::TimeUnits_t expectedMillisecondsTime = 500;
    const float expectedSecondsTime = 0.5f;

    time::TimeUnits_t resultMillisecondsTime = time::FromTo<time::TimeUnits_t>(expectedMicrosecondsTime, time::kMicrosecondUnit, time::kMilisecondUnit);
    CHECK_EQUAL(expectedMillisecondsTime, resultMillisecondsTime);

    float resultSecondsTime = time::FromTo<float>(expectedMicrosecondsTime, time::kMicrosecondUnit, time::kSecondUnit);
    CHECK_EQUAL(expectedSecondsTime, resultSecondsTime);

    resultSecondsTime = time::FromTo<float>(resultMillisecondsTime, time::kMilisecondUnit, time::kSecondUnit);
    CHECK_EQUAL(expectedSecondsTime, resultSecondsTime);

    resultMillisecondsTime = time::FromTo<time::TimeUnits_t>(resultSecondsTime, time::kSecondUnit, time::kMilisecondUnit);
    CHECK_EQUAL(expectedMillisecondsTime, resultMillisecondsTime);

    time::TimeUnits_t resultMicrosecondsTime = time::FromTo<time::TimeUnits_t>(resultSecondsTime, time::kSecondUnit, time::kMicrosecondUnit);
    CHECK_EQUAL(expectedMicrosecondsTime, resultMicrosecondsTime);

    resultMicrosecondsTime = time::FromTo<time::TimeUnits_t>(resultMillisecondsTime, time::kMilisecondUnit, time::kMicrosecondUnit);
    CHECK_EQUAL(expectedMicrosecondsTime, resultMicrosecondsTime);
}


//#include "Platform/Support.h"
//#include "LoggerCpp/Manager.h"
//#include "App/Args.h"
//#include "App/AppUtilities.h"
//#include "LoggerCpp/OutputDebug.h"
//#include "Fmt/ostream.h"
//#include "Exception/Exception.h"
//#include "Logger/YLog.h"
//#include "Platform/WindowsLean.h"
//#include "Time/GameClock.h"
//
//
////#include "TestReporterStdout.h"
////#include "CompositeTestReporter.h"
//#include "UnitTest/TestReporterOutputDebug.h"
//
//using namespace yaget;
//
//TEST(Sanity)
//{
//    CHECK_EQUAL(1, 1);
//}
//
//TEST(GameClock)
//{
//    time::GameClock gameClock;
//    gameClock.Tick(0);
//
//    double stampStart = platform::GetRealTime();
//    const float durationSeconds = 0.1f;
//    int sleepTime = time::FromTo<int>(durationSeconds, time::kSecondUnit, time::kMilisecondUnit);
//    platform::BusySleep(sleepTime);
//    double stampEnd = platform::GetRealTime();
//    double diff = stampEnd - stampStart;
//    CHECK_CLOSE(durationSeconds, diff, 0.0001f);
//}
//
//
//
//class TestOutputDebug : public ylog::Output
//{
//public:
//    TestOutputDebug() = default;
//    TestOutputDebug(const ylog::Config::Ptr& aConfigPtr);
//
//private:
//    void OnOutput(const ylog::Channel::Ptr& aChannelPtr, const ylog::Log& aLog) const override
//    {
//        const ylog::DateTime& time = aLog.getTime();
//        char buffer[512] = { '\0' };
//        char tag[5] = { '\0' };
//
//        if (aLog.GetTag())
//        {
//            *(reinterpret_cast<uint32_t*>(tag)) = aLog.GetTag();
//        }
//
//        _snprintf_s(buffer, sizeof(buffer), sizeof(buffer), "%s  %-12s [%s%s%s] %s(%d) %s\n", time.ToString().c_str(), aChannelPtr->getName().c_str(),
//            ylog::Log::toString(aLog.getSeverity()), tag ? ":" : "", tag,
//            aLog.GetFileName().c_str(), aLog.GetFileLine(), aLog.getStream().str().c_str());
//
//        buffer[sizeof(buffer) - 1] = '\0';
//        mLogLines.push_back(buffer);
//    }
//
//    mutable std::vector<std::string> mLogLines;
//};
//
//TestOutputDebug* testOutputDebug = nullptr;
//
//TestOutputDebug::TestOutputDebug(const ylog::Config::Ptr& /*aConfigPtr*/)
//{
//    testOutputDebug = this;
//}
//
////bool ParseOptions(const char* commandLine, args::Options& options)
////{
////    std::string errroMessage;
////    if (!platform::ParseArgs(commandLine, options, &errroMessage))
////    {
////        std::string message = fmt::format("{}\nOriginal command: '{}'\n\n{}", errroMessage, commandLine, options.help());
////        util::DisplayDialog("Yaget Command Line Options Error", message.c_str());
////        return false;
////    }
////
////    return true;
////}
//
////    int counter = 1;
////    YLOG_DEBUG("TEST", "%d - Output line goes here...", counter);
////    YLOG_INFO("TEST", "%d - Output line goes here...", counter);
////    YLOG_NOTICE("TEST", "%d - Output line goes here...", counter);
////    YLOG_WARNING("TEST", "%d - Output line goes here...", counter);
////    YLOG_ERROR("TEST", "%d - Output line goes here...", counter);
////    YLOG_CRITICAL("TEST", "%d - Output line goes here...", counter);
////
////    ylog::Get().setLevel(ylog::Log::eDebug);
////    counter++;
////    YLOG_DEBUG("TEST", "%d - Output line goes here...", counter);
////    YLOG_INFO("TEST", "%d - Output line goes here...", counter);
////    YLOG_NOTICE("TEST", "%d - Output line goes here...", counter);
////    YLOG_WARNING("TEST", "%d - Output line goes here...", counter);
////    YLOG_ERROR("TEST", "%d - Output line goes here...", counter);
////    YLOG_CRITICAL("TEST", "%d - Output line goes here...", counter);
////
////    ylog::Get().setLevel(ylog::Log::eInfo);
////    counter++;
////    YLOG_DEBUG("TEST", "%d - Output line goes here...", counter);
////    YLOG_INFO("TEST", "%d - Output line goes here...", counter);
////    YLOG_NOTICE("TEST", "%d - Output line goes here...", counter);
////    YLOG_WARNING("TEST", "%d - Output line goes here...", counter);
////    YLOG_ERROR("TEST", "%d - Output line goes here...", counter);
////    YLOG_CRITICAL("TEST", "%d - Output line goes here...", counter);
////
////    ylog::Get().setLevel(ylog::Log::eNotice);
////    counter++;
////    YLOG_DEBUG("TEST", "%d - Output line goes here...", counter);
////    YLOG_INFO("TEST", "%d - Output line goes here...", counter);
////    YLOG_NOTICE("TEST", "%d - Output line goes here...", counter);
////    YLOG_WARNING("TEST", "%d - Output line goes here...", counter);
////    YLOG_ERROR("TEST", "%d - Output line goes here...", counter);
////    YLOG_CRITICAL("TEST", "%d - Output line goes here...", counter);
////    ylog::Get().setLevel(ylog::Log::eWarning);
////    counter++;
////    YLOG_DEBUG("TEST", "%d - Output line goes here...", counter);
////    YLOG_INFO("TEST", "%d - Output line goes here...", counter);
////    YLOG_NOTICE("TEST", "%d - Output line goes here...", counter);
////    YLOG_WARNING("TEST", "%d - Output line goes here...", counter);
////    YLOG_ERROR("TEST", "%d - Output line goes here...", counter);
////    YLOG_CRITICAL("TEST", "%d - Output line goes here...", counter);
////    ylog::Get().setLevel(ylog::Log::eError);
////    counter++;
////    YLOG_DEBUG("TEST", "%d - Output line goes here...", counter);
////    YLOG_INFO("TEST", "%d - Output line goes here...", counter);
////    YLOG_NOTICE("TEST", "%d - Output line goes here...", counter);
////    YLOG_WARNING("TEST", "%d - Output line goes here...", counter);
////    YLOG_ERROR("TEST", "%d - Output line goes here...", counter);
////    YLOG_CRITICAL("TEST", "%d - Output line goes here...", counter);
//
//int main(int argc, char* argv[])
//{
//    platform::SetThreadName("unit_test.main()", platform::CurrentThreadId());
//
//    ylog::Manager::RegisterOutputType<ylog::OutputDebug>(&ylog::CreateOutputInstance<ylog::OutputDebug>);
//
//    args::Options options("YagetCore.UnitTest", "Unit test of yaget core library");
//    if (system::InitializeSetup("--output=TestOutputDebug --output=ylog::OutputDebug --log_level=CRIT", options) != system::OptionResult::OK)
//    {
//        return -1;
//    }
//
//    UnitTest::TestReporterStdout reporterSTDOUT;
//    UnitTest::CompositeTestReporter compositeTestReporter;
//    compositeTestReporter.AddReporter(&reporterSTDOUT);
//
//    TestReporterOutputDebug reporterOD;
//    if (platform::IsDebuggerAttached())
//    {
//        compositeTestReporter.AddReporter(&reporterOD);
//    }
//
//    UnitTest::TestRunner runner(compositeTestReporter);
//    int result = runner.RunTestsIf(UnitTest::Test::GetTestList(), nullptr, UnitTest::True(), 0);
//
//    return result;
//}

