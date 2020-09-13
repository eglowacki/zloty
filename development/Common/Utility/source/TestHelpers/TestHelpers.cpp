#include "TestHelpers/TestHelpers.h"
#include "App/Application.h"
#include "LoggerCpp/OutputDebug.h"
#include "LoggerCpp/OutputFile.h"


namespace
{
    const auto configBlock = R"(
        {
            "Configuration": {
                "Debug": {

                    "Logging": {
                        "Filters": [],
                        "Level": "DBUG",
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


void yaget::test::InitializeEnvironment()
{
    ylog::Manager::RegisterOutputTypes<ylog::OutputDebug, ylog::OutputFile>();

    system::InitializeSetup(configBlock, std::strlen(configBlock), true);
    metrics::MarkStartThread(platform::CurrentThreadId(), "MAIN");
}

void yaget::test::ResetEnvironment()
{
    ylog::Manager::ResetRuntimeData();
}

