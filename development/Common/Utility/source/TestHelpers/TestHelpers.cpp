#include "TestHelpers/TestHelpers.h"
#include "App/Application.h"



#include "Json/JsonHelpers.h"

#include "LoggerCpp/OutputDebug.h"
#include "LoggerCpp/OutputFile.h"
#include "Debugging/DevConfiguration.h"
#include "Debugging/DevConfigurationParsers.h"



namespace
{
    const auto defaultConfigBlock = R"(
        {
            "Configuration": {
                "Debug": {
                    "Logging": {
                        "Filters": [],
                        "Level": "NOTE",
                        "Outputs": {
                            "ylog::OutputDebug": {
                                "split_lines": "true"
                            },
                            "ylog::OutputFile": {
                                "split_lines": "false",
                                "max_startup_size": "0",
                                "filename": "$(LogFolder)/$(AppName).log"
                            }
                        }
                    }
                }
            }
        }
    )";

}


void yaget::test::InitializeEnvironment(const char* configBlockData /*= nullptr*/, std::size_t size /*= 0*/)
{
    platform::DisregardAttachedDebugger();
    // since this is run only in test environment, do not output to console (cout, cerr)
    // let debugger catch log messages and allow file log
    ylog::Manager::RegisterOutputTypes<ylog::OutputDebug, ylog::OutputFile>();

    // process default settings for configuration and make simple validation
    nlohmann::json jsonBlock = nlohmann::json::parse(defaultConfigBlock);
    YAGET_ASSERT(json::IsSectionValid(jsonBlock, "Configuration", ""), "defaultConfigBlock missing 'Configuration' section.");

    nlohmann::json& configurationBlock = jsonBlock["Configuration"];

    dev::Configuration configuration;
    dev::from_json(configurationBlock, configuration);

    if (configBlockData && size)
    {
        nlohmann::json userBlock = nlohmann::json::parse(std::string(configBlockData, size));
        YAGET_ASSERT(json::IsSectionValid(userBlock, "Configuration", ""), "userConfigBlock missing 'Configuration' section.");

        dev::from_json(userBlock["Configuration"], configuration);
    }

    dev::to_json(configurationBlock, configuration);
    std::string block = json::PrettyPrint(jsonBlock);

    system::InitializeSetup(block.c_str(), block.size(), true);
    metrics::MarkStartThread(platform::CurrentThreadId(), "Main");
}

void yaget::test::ResetEnvironment()
{
    ylog::Manager::ResetRuntimeData();
}

