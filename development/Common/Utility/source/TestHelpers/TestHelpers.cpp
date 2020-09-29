#include "TestHelpers/TestHelpers.h"
#include "App/Application.h"



#include "Json/JsonHelpers.h"

#include "LoggerCpp/OutputDebug.h"
#include "LoggerCpp/OutputFile.h"
#include "Debugging/DevConfiguration.h"
#include "Debugging/DevConfigurationParsers.h"



namespace
{
    const auto configBlock = R"(
        {
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
    )";

}


void yaget::test::InitializeEnvironment(const char* configBlockData /*= nullptr*/, std::size_t size /*= 0*/)
{
    ylog::Manager::RegisterOutputTypes<ylog::OutputDebug, ylog::OutputFile>();

    dev::Configuration configuration;
    nlohmann::json systemBlock = nlohmann::json::parse(configBlock);
    auto p = json::PrettyPrint(systemBlock);

    dev::from_json(systemBlock, configuration);

    if (configBlockData && size)
    {
        nlohmann::json userBlock = nlohmann::json::parse(configBlockData);
        dev::from_json(userBlock, configuration);
    }
    
    nlohmann::json mergedConfig;
    dev::to_json(mergedConfig["Configuration"], configuration);
    std::string block = json::PrettyPrint(mergedConfig);

    system::InitializeSetup(block.c_str(), block.size(), true);
    metrics::MarkStartThread(platform::CurrentThreadId(), "MAIN");
}

void yaget::test::ResetEnvironment()
{
    ylog::Manager::ResetRuntimeData();
}

