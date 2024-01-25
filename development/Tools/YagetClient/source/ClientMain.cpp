#include "Client.h"

#include "YagetVersion.h"
#include "App/AppHarness.h"

#include "VTS/DiagnosticVirtualTransportSystem.h"
#include "LoggerCpp/OutputDebug.h"
#include "LoggerCpp/OutputFile.h"
#include "LoggerCpp/OutputConsole.h"

#include "MemoryManager/PoolAllocator.h"
#include "MemoryManager/NewAllocator.h"

#include "Metrics/Concurrency.h"

yaget::Strings yaget::ylog::GetRegisteredTags()
{
    yaget::Strings tags =
    {
        #include "Logger/LogTags.h"
        "SPAM",
        "CLNT",
    };

    return tags;
}

YAGET_BRAND_NAME_F("Beyond Limits")
YAGET_CUSTOMIZE_STRIP_KEYWORDS(",::client,client::")


//---------------------------------------------------------------------------------------------------------------------
int main(int argc, char* argv[])
{
    YAGET_CHECKVERSION;

    using namespace yaget;

    memory::InitializeAllocations();

    args::Options options("Yaget.Client", "Yaget Client.");
    options.add_options()
        ("a,address", "Specifies which server connect to. IP:Port.")
    ;

    const int result = app::helpers::Harness<ylog::OutputFile, ylog::OutputConsole, ylog::OutputDebug>(argc, argv, options, nullptr, 0, [&options]()
    {
        metrics::Channel channel("Main.Client", YAGET_METRICS_CHANNEL_FILE_LINE);

        if (options.find<bool>("vts_fix", false))
        {
            yaget::io::diag::VirtualTransportSystem vtsFixer(false, "$(DatabaseFolder)/vts.sqlite");
        }

        return client::Run(options);
    });

    memory::ReportAllocations();

    return result;
}
