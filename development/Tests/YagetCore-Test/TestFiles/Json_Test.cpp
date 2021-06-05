#include "pch.h" 
#include "Debugging/DevConfiguration.h"
#include "Debugging/DevConfigurationParsers.h"
#include "TestHelpers/TestHelpers.h"
#include "Json/JsonHelpers.h"


//CHECK_EQUAL(expected, actual);
class JsonUtilities : public ::testing::Test
{
};


TEST_F(JsonUtilities, ConfigString)
{
    using namespace yaget;

    dev::Configuration expectedTestconfiguration1;
    expectedTestconfiguration1.mDebug.mMetrics.TraceOn = false;

    dev::Configuration expectedTestconfiguration2;
    expectedTestconfiguration2.mDebug.mMetrics.TraceFileName = "Foo.trc";

    dev::Configuration expectedTestconfiguration4;
    expectedTestconfiguration4.mDebug.mMetrics.TraceFileName = "";

    const char* testString1 = "Debug.Metrics.TraceOn=false";
    const char* testString2 = "Debug.Metrics.TraceFileName = 'Foo.trc'";
    const char* testString3 = "Debug.Metrics.TraceFileName='Foo.trc'";
    const char* testString4 = "Debug.Metrics.TraceFileName=''";
    const char* testString5 = "Debug.Metrics.TraceFileName";

    const auto jsonBlock1 = json::ParseConfig(testString1);
    const auto jsonBlock2 = json::ParseConfig(testString2);
    const auto jsonBlock3 = json::ParseConfig(testString3);
    const auto jsonBlock4 = json::ParseConfig(testString4);
    const auto jsonBlock5 = json::ParseConfig(testString5);

    dev::Configuration configuration;
    EXPECT_TRUE(configuration.mDebug.mMetrics.TraceOn);
    from_json(jsonBlock1, configuration);
    EXPECT_EQ(expectedTestconfiguration1, configuration);

    configuration = {};
    EXPECT_STREQ(configuration.mDebug.mMetrics.TraceFileName.c_str(), "$(Temp)/$(AppName)_trace.json");
    from_json(jsonBlock2, configuration);
    EXPECT_EQ(expectedTestconfiguration2, configuration);

    configuration = {};
    EXPECT_STREQ(configuration.mDebug.mMetrics.TraceFileName.c_str(), "$(Temp)/$(AppName)_trace.json");
    from_json(jsonBlock3, configuration);
    EXPECT_EQ(expectedTestconfiguration2, configuration);

    configuration = {};
    EXPECT_STREQ(configuration.mDebug.mMetrics.TraceFileName.c_str(), "$(Temp)/$(AppName)_trace.json");
    from_json(jsonBlock4, configuration);
    EXPECT_EQ(expectedTestconfiguration4, configuration);

    configuration = {};
    EXPECT_STREQ(configuration.mDebug.mMetrics.TraceFileName.c_str(), "$(Temp)/$(AppName)_trace.json");
    from_json(jsonBlock5, configuration);
    EXPECT_STREQ(configuration.mDebug.mMetrics.TraceFileName.c_str(), "$(Temp)/$(AppName)_trace.json");
}
