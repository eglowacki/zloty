// YagetCoreUnitTest.cpp : Defines the entry point for the console application.
//

#include "YagetCore.h"
#include "UnitTest++.h"
#include "Debugging/DevConfiguration.h"
#include "App/Args.h"
#include "App/AppUtilities.h"
#include "Fmt/ostream.h"
#include "Exception/Exception.h"
#include "LoggerCpp/Manager.h"
#include "LoggerCpp/OutputDebug.h"
#include "LoggerCpp/OutputConsole.h"
//#include "Logger/YLog.h"
#include "YagetVersion.h"
#include "Platform/WindowsLean.h"
#include "Platform/Support.h"
#include "Time/GameClock.h"
#include "UnitTest/TestReporterOutputDebug.h"

#include <filesystem>
namespace fs = std::filesystem;

#pragma message(YAGET_COMPILER_INFO)

using namespace yaget;


namespace yaget::ylog
{
    yaget::Strings GetRegisteredTags()
    {
        yaget::Strings tags =
        {
            #include "Logger/LogTags.h"
        };

        return tags;
    }

} //namespace yaget::ylog

class TestOutputDebug : public ylog::Output
{
public:
    TestOutputDebug() = default;
    TestOutputDebug(const ylog::Config::Ptr& aConfigPtr);

private:
    void OnOutput(const ylog::Channel::Ptr& aChannelPtr, const ylog::Log& aLog) const override
    {
        const ylog::DateTime& time = aLog.getTime();
        char buffer[512] = { '\0' };
        char tag[5] = { '\0' };

        if (aLog.GetTag())
        {
            *(reinterpret_cast<uint32_t*>(tag)) = aLog.GetTag();
        }

        _snprintf_s(buffer, sizeof(buffer), sizeof(buffer), "%s  %-12s [%s%s%s] %s(%d) %s : %s\n", time.ToString().c_str(), aChannelPtr->getName().c_str(),
            ylog::Log::toString(aLog.getSeverity()), tag ? ":" : "", tag,
            aLog.GetFileName().c_str(), aLog.GetFileLine(), aLog.getStream().str().c_str(), aLog.GetFunctionName().c_str());

        buffer[sizeof(buffer) - 1] = '\0';
        mLogLines.push_back(buffer);
    }

    mutable std::vector<std::string> mLogLines;
};

TestOutputDebug* testOutputDebug = nullptr;

TestOutputDebug::TestOutputDebug(const ylog::Config::Ptr& /*aConfigPtr*/)
{
    testOutputDebug = this;
}


int main(int argc, char* argv[])
{
    using namespace yaget;

    dev::CurrentConfiguration().mDebug.Refresh(true, 1);

    platform::SetThreadName("y.main", platform::CurrentThreadId());

    ylog::Manager::RegisterOutputType<ylog::OutputDebug>(&ylog::CreateOutputInstance<ylog::OutputDebug>);
    ylog::Manager::RegisterOutputType<ylog::OutputConsole>(&ylog::CreateOutputInstance<ylog::OutputConsole>);

    args::Options options("YagetCore.UnitTest", "Unit test of yaget core library");
    options.add_options()
        ("test_filter", "Name of test to run.", args::value<std::string>())
        ;

    const char* configData = nullptr;
    size_t configSize = 0;

    if (system::InitializeSetup(argc, argv, options, configData, configSize) != system::InitializationResult::OK)
    {
        return -1;
    }

    UnitTest::TestReporterStdout reporterSTDOUT;
    UnitTest::CompositeTestReporter compositeTestReporter;
    compositeTestReporter.AddReporter(&reporterSTDOUT);

    TestReporterOutputDebug reporterOD;
    if (platform::IsDebuggerAttached())
    {
        compositeTestReporter.AddReporter(&reporterOD);
    }

    UnitTest::TestList newList;

    std::string requestedTest = options.find<std::string>("test_filter", "");

    using customFilter_t = std::function<bool(const UnitTest::Test* const test)>;
    customFilter_t customFilter = [&requestedTest](const UnitTest::Test* test) {return CompareI(requestedTest, test->m_details.testName); };
    customFilter_t callback = requestedTest.empty() ? UnitTest::True() : customFilter;

    UnitTest::TestRunner runner(compositeTestReporter);
    int result = runner.RunTestsIf(UnitTest::Test::GetTestList(), nullptr, callback, 0);

    return result;
}

