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
    ylog::Manager::RegisterOutputTypes<ylog::OutputDebug, ylog::OutputFile>();

    const char* block = configBlockData ? configBlockData : configBlock;
    const std::size_t blockSize = configBlockData ? size : std::strlen(configBlock);
    system::InitializeSetup(block, blockSize, true);
    metrics::MarkStartThread(platform::CurrentThreadId(), "MAIN");
}

void yaget::test::ResetEnvironment()
{
    ylog::Manager::ResetRuntimeData();
}

