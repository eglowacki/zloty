#include "Server.h"

#include "YagetVersion.h"
#include "App/AppHarness.h"

#include "LoggerCpp/OutputConsole.h"
#include "LoggerCpp/OutputDebug.h"
#include "LoggerCpp/OutputFile.h"
#include "MemoryManager/NewAllocator.h"
#include "VTS/DiagnosticVirtualTransportSystem.h"

#include "Metrics/Concurrency.h"

yaget::Strings yaget::ylog::GetRegisteredTags()
{
    yaget::Strings tags =
    {
        #include "Logger/CoreLogTags.h"
        "SERV",
    };

    return tags;
}

YAGET_BRAND_NAME_F("Beyond Limits")
YAGET_CUSTOMIZE_STRIP_KEYWORDS(",::server,server::")


//---------------------------------------------------------------------------------------------------------------------
int main(int argc, char* argv[])
{
    YAGET_CHECKVERSION;

    using namespace yaget;

    memory::InitializeAllocations();

    args::Options options("Yaget.Server", "Yaget Server.");
    options.add_options()
        ("p,port", "Specifies which port server is using for connection.", args::value<int>())
    ;

    const int result = app::helpers::Harness<ylog::OutputFile, ylog::OutputConsole, ylog::OutputDebug>(argc, argv, options, nullptr, 0, [&options]()
    {
        metrics::Channel channel("Main.Server", YAGET_METRICS_CHANNEL_FILE_LINE);

        if (options.find<bool>("vts_fix", false))
        {
            yaget::io::diag::VirtualTransportSystem vtsFixer(false, "$(DatabaseFolder)/vts.sqlite");
        }

        return server::Run(options);
    });

    memory::ReportAllocations();

    return result;
}
