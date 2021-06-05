#include "pch.h"
#include "Debugging/DevConfiguration.h"
#include "Debugging/DevConfigurationParsers.h"
#include "TestHelpers/TestHelpers.h"

//CHECK_EQUAL(expected, actual);

class Configurator : public ::testing::Test
{
protected:
    // void SetUp() override {}
    // void TearDown() override {}

private:
    //yaget::test::Environment mEnvironment;
};


namespace
{
    // helper function to copy from T to json
    // and back to T and return that
    template <typename T>
    T ReCopy(const T& original)
    {
        nlohmann::json createdBlock;
        to_json(createdBlock, original);
        T copy{};
        from_json(createdBlock, copy);

        return copy;
    }

    // helper function to copy from json T return that
    template <typename T>
    T Construct(const nlohmann::json& jsonBlock)
    {
        T copy{};
        from_json(jsonBlock, copy);

        return copy;
    }
}

namespace json_blocks
{
    using namespace yaget::dev;

    // this provides const data to test on and with.
    // Each section has two data objects, one for json block, filled with some arbitrary test values
    // and second for c++ structure filled with matching values.
    // The test flow is: call with from_json(...) of this json block located in this namespace
    // and with your c++ structure matching data, then check your c++ structure against one in this <namespace::expected*>
    //
    // Configuration::Debug::Flags actualFlags;
    // from_json(json_blocks::flags, actualFlags);
    // CHECK(json_blocks::expectedFlags == actualFlags);

    //------------------------------------------------------------------------------------------------------------------------------------------------------
    const nlohmann::json flags = R"({
        "Physics": false,
        "Gui": false,
        "SuppressUI": true,
        "BuildId": 417,
        "MetricGather": true
    })"_json;

    const Configuration::Debug::Flags expectedFlags =
    {
        false,
        false,
        true,
        417,
        true
    };

    //------------------------------------------------------------------------------------------------------------------------------------------------------
    const nlohmann::json logging = R"({
        "Level" : "EROR",
        "Filters": [ "!PROF" ],
        "Outputs" : {
            "Sink_One": null,
            "Sink_Two" : null,
            "Sink_Three" : { "Key_One": "Value_One" }
        }
    })"_json;

    const Configuration::Debug::Logging expectedLogging =
    {
        "EROR",
        { "!PROF" },
        {
            { "Sink_One", {} },
            { "Sink_Two", {} },
            { "Sink_Three", { { "Key_One", "Value_One" } } }
        }
    };

    //------------------------------------------------------------------------------------------------------------------------------------------------------
    // Combine 'logging' and 'loggingCombineOne' into one and test it with 'expectedLoggingCombineOne' 
    const nlohmann::json loggingCombineOne = R"({
        "Filters": [ "TAG1", "TAG2" ],
        "Outputs" : {
            "Sink_Two" : { "MyKey": "MyValue" },
            "Sink_Three" : { "Key_One": "NewValue_Two" },
            "MySink_Four": null
        }
    })"_json;

    // combination of 'logging' and 'loggingCombineOne' should produce this struct
    const Configuration::Debug::Logging expectedLoggingCombineOne =
    {
        "EROR",
        { "!PROF", "TAG1", "TAG2" },
        {
            { "Sink_One", {} },
            { "Sink_Two", { { "MyKey", "MyValue" } } },
            { "Sink_Three", { { "Key_One", "NewValue_Two" } } },
            { "MySink_Four", {} }
        }
    };

    //------------------------------------------------------------------------------------------------------------------------------------------------------
    // Combine 'logging+loggingCombineOne' and 'loggingCombineTwo' into one and test it with 'expectedLoggingCombineTwo' 
    const nlohmann::json loggingCombineTwo = R"({
        "Filters": [ "!PROF", "-TAG2" ]
    })"_json;

    const Configuration::Debug::Logging expectedLoggingCombineTwo =
    {
        json_blocks::expectedLoggingCombineOne.Level,
        { "!PROF", "TAG1" },
        json_blocks::expectedLoggingCombineOne.Outputs
    };

    //------------------------------------------------------------------------------------------------------------------------------------------------------
    const nlohmann::json threads = R"({
        "VTSSections": 1,
        "VTS": 2,
        "Blob": 3,
        "App": 4
    })"_json;

    const Configuration::Debug::Threads expectedThreads = { 1, 2, 3, 4 };

    //------------------------------------------------------------------------------------------------------------------------------------------------------
    const nlohmann::json metrics = R"({
        "AllowSocketConnection": true,
        "AllowFallbackToFile": true,
        "SocketConnectionTimeout": -1,
        "TraceFileName": "FooFileName",
        "TraceOn": false
    })"_json;

    const Configuration::Debug::Metrics expectedMetrics = { true, true, -1, "FooFileName", false };

    //------------------------------------------------------------------------------------------------------------------------------------------------------
    const nlohmann::json debug =
    {
        { "Flags", json_blocks::flags },
        { "Logging", json_blocks::logging },
        { "Threads", json_blocks::threads },
        { "Metrics", json_blocks::metrics }
    };

    const Configuration::Debug expectedDebug =
    {
        json_blocks::expectedFlags,
        json_blocks::expectedLogging,
        json_blocks::expectedThreads,
        json_blocks::expectedMetrics
    };

    //------------------------------------------------------------------------------------------------------------------------------------------------------
    const nlohmann::json debugCombineOne =
    {
        { "Logging", json_blocks::loggingCombineOne }
    };

    const Configuration::Debug expectedDebugCombineOne =
    {
        json_blocks::expectedFlags,
        json_blocks::expectedLoggingCombineOne,
        json_blocks::expectedThreads,
        json_blocks::expectedMetrics
    };

    //------------------------------------------------------------------------------------------------------------------------------------------------------
    const nlohmann::json debugCombineTwo =
    {
        { "Logging", json_blocks::loggingCombineTwo }
    };

    const Configuration::Debug expectedDebugCombineTwo =
    {
        json_blocks::expectedFlags,
        json_blocks::expectedLoggingCombineTwo,
        json_blocks::expectedThreads,
        json_blocks::expectedMetrics
    };

    //------------------------------------------------------------------------------------------------------------------------------------------------------
    const nlohmann::json vtsConfig = R"([
        {
            "BinTester3": {
                "Path": [ "$(Temp)/section-11" ],
                "Filters": [ "*.bin" ],
                "Converters": "BINNER",
                "ReadOnly": false,
                "Recursive": true
            }
        }
    ])"_json;

    const Configuration::Init::VTSConfigList expectedVTSConfig =
    {
        {
            "BinTester3",
            { "$(Temp)/section-11" },
            { "*.bin" },
            "BINNER",
            false,
            true
        }
    };

    //------------------------------------------------------------------------------------------------------------------------------------------------------
    const nlohmann::json vtsConfigCombineOne = R"([
        {
            "BinTester3": {
                "Path": [ "$(Temp)/section-417" ],
                "Filters": [ "*.glo" ]
            }
        }
    ])"_json;

    const Configuration::Init::VTSConfigList expectedVTSConfigCombineOne =
    {
        {
            "BinTester3",
            { "$(Temp)/section-11", "$(Temp)/section-417" },
            { "*.bin", "*.glo" },
            "BINNER",
            false,
            true
        }
    };

    //------------------------------------------------------------------------------------------------------------------------------------------------------
    const nlohmann::json vtsConfigCombineTwo = R"([
        {
            "BinTester6": {
                "Path": [ "$(Temp)/foo-417" ],
                "Filters": [ "*.bar" ],
                "Converters": "FREEK",
                "ReadOnly": false,
                "Recursive": false
            }
        }
    ])"_json;

    const Configuration::Init::VTSConfigList expectedVTSConfigCombineTwo =
    {
        {
            "BinTester3",
            { "$(Temp)/section-11", "$(Temp)/section-417" },
            { "*.bin", "*.glo" },
            "BINNER",
            false,
            true
        },
        {
            "BinTester6",
            { "$(Temp)/foo-417" },
            { "*.bar" },
            "FREEK",
            false,
            false
        }
    };

    //------------------------------------------------------------------------------------------------------------------------------------------------------
    const nlohmann::json environment = R"({
        "$AliasOne": { "Path": "Folder1", "ReadOnly": true },
        "$AliasTwo": { "Path": "Folder2", "ReadOnly":  false }
    })"_json;

    const Configuration::Init::EnvironmentList expectedEnvironment =
    {
        { "$AliasOne", { "Folder1", true } },
        { "$AliasTwo", { "Folder2", false } }
    };

    //------------------------------------------------------------------------------------------------------------------------------------------------------
    const nlohmann::json environmentCombineOne = R"({
        "$AliasThree": { "Path": "Folder3", "ReadOnly":  false }
    })"_json;

    const Configuration::Init::EnvironmentList expectedEnvironmentCombineOne =
    {
        { "$AliasOne", { "Folder1", true } },
        { "$AliasTwo", { "Folder2", false } },
        { "$AliasThree", { "Folder3", false } }
    };

    //------------------------------------------------------------------------------------------------------------------------------------------------------
    const nlohmann::json environmentCombineTwo = R"({
        "$AliasTwo": { "Path": "MyFolder", "ReadOnly":  true }
    })"_json;

    const Configuration::Init::EnvironmentList expectedEnvironmentCombineTwo =
    {
        { "$AliasOne", { "Folder1", true } },
        { "$AliasTwo", { "MyFolder", true } },
        { "$AliasThree", { "Folder3", false } }
    };

    //------------------------------------------------------------------------------------------------------------------------------------------------------
    const nlohmann::json init =
    {
        { "VTS", json_blocks::vtsConfig },
        { "Aliases", json_blocks::environment },
        { "WindowOptions", "UserOptions@window" },
        { "GameDirectorScript", "Scripts@Ponger" }
    };

    const Configuration::Init expectedInit =
    {
        false,
        false,
        1920,
        1080,
        yaget::time::kFrames_60,
        json_blocks::expectedVTSConfig,
        json_blocks::expectedEnvironment,
        "UserOptions@window",
        "Scripts@Ponger"
    };

    //------------------------------------------------------------------------------------------------------------------------------------------------------
    const nlohmann::json initCombineOne =
    {
        { "VTS", json_blocks::vtsConfigCombineOne },
        { "Aliases", json_blocks::environmentCombineOne }
    };

    //------------------------------------------------------------------------------------------------------------------------------------------------------
    const Configuration::Init expectedInitCombineOne =
    {
        false,
        false,
        1920,
        1080,
        yaget::time::kFrames_60,
        json_blocks::expectedVTSConfigCombineOne,
        json_blocks::expectedEnvironmentCombineOne,
        "UserOptions@window",
        "Scripts@Ponger"
    };

    //------------------------------------------------------------------------------------------------------------------------------------------------------
    const nlohmann::json runtime =
    {
        { "DpiScaleFactor", 417 },
        { "ShowScriptHelp", true }
    };

    const Configuration::Runtime expectedRuntime =
    {
        417,
        true
    };

    //------------------------------------------------------------------------------------------------------------------------------------------------------
    const nlohmann::json graphics =
    {
        { "Device", "Garbage" },
        { "MemoryReport", true }
    };

    const Configuration::Graphics expectedGraphics =
    {
        "Garbage",
        true
    };

    //------------------------------------------------------------------------------------------------------------------------------------------------------
    const nlohmann::json configuration =
    {
        { "Debug", json_blocks::debug },
        { "Init", json_blocks::init },
        { "Runtime", json_blocks::runtime },
        { "Graphics", json_blocks::graphics }
    };

    const Configuration expectedConfiguration =
    {
        json_blocks::expectedDebug,
        json_blocks::expectedInit,
        json_blocks::expectedRuntime,
        json_blocks::expectedGraphics
    };

    //------------------------------------------------------------------------------------------------------------------------------------------------------
    const nlohmann::json configurationCombineOne =
    {
        { "Debug", json_blocks::debugCombineOne },
        { "Init", json_blocks::initCombineOne }
    };

    const Configuration expectedConfigurationCombineOne =
    {
        json_blocks::expectedDebugCombineOne,
        json_blocks::expectedInitCombineOne,
        json_blocks::expectedRuntime,
        json_blocks::expectedGraphics
    };

    //------------------------------------------------------------------------------------------------------------------------------------------------------
    const nlohmann::json configurationCombineTwo =
    {
        { "Debug", json_blocks::debugCombineTwo }
    };

    const Configuration expectedConfigurationCombineTwo =
    {
        json_blocks::expectedDebugCombineTwo,
        json_blocks::expectedInitCombineOne,
        json_blocks::expectedRuntime,
        json_blocks::expectedGraphics
    };

} // namespace json_blocks


//------------------------------------------------------------------------------------------------------------------------------------------------------
TEST_F(Configurator, Flags)
{
    using namespace yaget;
    using namespace yaget::dev;

    Configuration::Debug::Flags actualFlags;

    const nlohmann::json& jsonBlock = json_blocks::flags;

    from_json(jsonBlock, actualFlags);
    EXPECT_EQ(json_blocks::expectedFlags, actualFlags);

    actualFlags = ReCopy(json_blocks::expectedFlags);
    EXPECT_EQ(json_blocks::expectedFlags, actualFlags);
}


TEST_F(Configurator, Logging)
{
    using namespace yaget;
    using namespace yaget::dev;

    Configuration::Debug::Logging actualLogging;

    // test full Logging block
    const nlohmann::json& jsonBlock = json_blocks::logging;

    from_json(jsonBlock, actualLogging);
    EXPECT_EQ(json_blocks::expectedLogging, actualLogging);

    from_json(json_blocks::loggingCombineOne, actualLogging);
    EXPECT_EQ(json_blocks::expectedLoggingCombineOne, actualLogging);

    from_json(json_blocks::loggingCombineTwo, actualLogging);
    EXPECT_EQ(json_blocks::expectedLoggingCombineTwo, actualLogging);

    actualLogging = ReCopy(json_blocks::expectedLogging);
    EXPECT_EQ(json_blocks::expectedLogging, actualLogging);
}


TEST_F(Configurator, Threads)
{
    using namespace yaget;
    using namespace yaget::dev;

    Configuration::Debug::Threads actualThreads;

    const nlohmann::json& jsonBlock = json_blocks::threads;

    from_json(jsonBlock, actualThreads);
    EXPECT_EQ(json_blocks::expectedThreads, actualThreads);

    actualThreads = ReCopy(json_blocks::expectedThreads);
    EXPECT_EQ(json_blocks::expectedThreads, actualThreads);
}



TEST_F(Configurator, Metrics)
{
    using namespace yaget;
    using namespace yaget::dev;

    auto actualMetrics = ReCopy(json_blocks::expectedMetrics);
    EXPECT_EQ(json_blocks::expectedMetrics, actualMetrics);

    actualMetrics = Construct<Configuration::Debug::Metrics>(json_blocks::metrics);
    EXPECT_EQ(json_blocks::expectedMetrics, actualMetrics);
}


TEST_F(Configurator, Debug)
{
    using namespace yaget;
    using namespace yaget::dev;

    Configuration::Debug actualDebug;

    const nlohmann::json& jsonBlock = json_blocks::debug;

    from_json(jsonBlock, actualDebug);
    EXPECT_EQ(json_blocks::expectedDebug, actualDebug);

    from_json(json_blocks::debugCombineOne, actualDebug);
    EXPECT_EQ(json_blocks::expectedDebugCombineOne, actualDebug);

    from_json(json_blocks::debugCombineTwo, actualDebug);
    EXPECT_EQ(json_blocks::expectedDebugCombineTwo, actualDebug);

    actualDebug = ReCopy(json_blocks::expectedDebug);
    EXPECT_EQ(json_blocks::expectedDebug, actualDebug);
}


TEST_F(Configurator, VTSConfig)
{
    using namespace yaget;
    using namespace yaget::dev;

    Configuration::Init::VTSConfigList actualVTSConfig;

    const nlohmann::json& jsonBlock = json_blocks::vtsConfig;

   from_json(jsonBlock, actualVTSConfig);
   EXPECT_EQ(json_blocks::expectedVTSConfig, actualVTSConfig);

   from_json(json_blocks::vtsConfigCombineOne, actualVTSConfig);
   EXPECT_EQ(json_blocks::expectedVTSConfigCombineOne, actualVTSConfig);

   from_json(json_blocks::vtsConfigCombineTwo, actualVTSConfig);
   EXPECT_EQ(json_blocks::expectedVTSConfigCombineTwo, actualVTSConfig);
}


TEST_F(Configurator, Environment)
{
    using namespace yaget;
    using namespace yaget::dev;

    Configuration::Init::EnvironmentList actualEnvironment;

    const nlohmann::json& jsonBlock = json_blocks::environment;

    from_json(jsonBlock, actualEnvironment);
    EXPECT_EQ(json_blocks::expectedEnvironment, actualEnvironment);

    from_json(json_blocks::environmentCombineOne, actualEnvironment);
    EXPECT_EQ(json_blocks::expectedEnvironmentCombineOne, actualEnvironment);

    from_json(json_blocks::environmentCombineTwo, actualEnvironment);
    EXPECT_EQ(json_blocks::expectedEnvironmentCombineTwo, actualEnvironment);
}



TEST_F(Configurator, Init)
{
    using namespace yaget;
    using namespace yaget::dev;

    Configuration::Init actualInit;

    const nlohmann::json& jsonBlock = json_blocks::init;

    from_json(jsonBlock, actualInit);
    EXPECT_EQ(json_blocks::expectedInit, actualInit);

    from_json(json_blocks::initCombineOne, actualInit);
    EXPECT_EQ(json_blocks::expectedInitCombineOne, actualInit);

    actualInit = ReCopy(json_blocks::expectedInit);
    EXPECT_EQ(json_blocks::expectedInit, actualInit);
}


TEST_F(Configurator, Runtime)
{
    using namespace yaget;
    using namespace yaget::dev;

    Configuration::Runtime actualRuntime;

    const nlohmann::json& jsonBlock = json_blocks::runtime;

    from_json(jsonBlock, actualRuntime);
    EXPECT_EQ(json_blocks::expectedRuntime, actualRuntime);

    actualRuntime = ReCopy(json_blocks::expectedRuntime);
    EXPECT_EQ(json_blocks::expectedRuntime, actualRuntime);
}


TEST_F(Configurator, Graphics)
{
    using namespace yaget;
    using namespace yaget::dev;

    Configuration::Graphics actualGraphics;

    const nlohmann::json& jsonBlock = json_blocks::graphics;

    from_json(jsonBlock, actualGraphics);
    EXPECT_EQ(json_blocks::expectedGraphics, actualGraphics);

    actualGraphics = ReCopy(json_blocks::expectedGraphics);
    EXPECT_EQ(json_blocks::expectedGraphics, actualGraphics);
}


TEST_F(Configurator, GuiColors)
{
    // TODO: add test for this map of Colors, keyed on string
}


TEST_F(Configurator, Compirison)
{
    using namespace yaget;
    using namespace yaget::dev;

    Configuration actualConfiguration;

    const nlohmann::json& jsonBlock = json_blocks::configuration;

    from_json(jsonBlock, actualConfiguration);
    EXPECT_EQ(json_blocks::expectedConfiguration, actualConfiguration);

    from_json(json_blocks::configurationCombineOne, actualConfiguration);
    EXPECT_EQ(json_blocks::expectedConfigurationCombineOne, actualConfiguration);

    from_json(json_blocks::configurationCombineTwo, actualConfiguration);
    EXPECT_EQ(json_blocks::expectedConfigurationCombineTwo, actualConfiguration);
}


TEST_F(Configurator, DefaultInit)
{
    using namespace yaget;
    using namespace yaget::dev;

    Configuration configuration;
    nlohmann::json jsonBlock;
    to_json(jsonBlock, configuration);

    Configuration copiedConfiguration;
    from_json(jsonBlock, copiedConfiguration);

    EXPECT_EQ(configuration, copiedConfiguration);
}
