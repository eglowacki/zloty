#include "YagetCore.h"
#include "YagetVersion.h"
#include "App/AppHarness.h"
#include "VTS/DiagnosticVirtualTransportSystem.h"
#include "LoggerCpp/OutputDebug.h"
#include "LoggerCpp/OutputFile.h"
#include "Metrics/Concurrency.h"

namespace yaget::ylog
{
    yaget::Strings GetRegisteredTags()
    {
        yaget::Strings tags =
        {
            #include "Logger/LogTags.h"
            "SPAM",
            "EDIT"
        };

        return tags;
    }
}

YAGET_BRAND_NAME_F("Beyond Limits")
YAGET_CUSTOMIZE_STRIP_KEYWORDS(",::editor,editor::")


int main(int argc, char* argv[])
{
    YAGET_CHECKVERSION;

    using namespace yaget;
    using Section = io::VirtualTransportSystem::Section;

    args::Options options("Yaget.Editor", "Yaget Editor.");

    const int result = app::helpers::Harness<ylog::OutputFile, ylog::OutputDebug>(argc, argv, options, nullptr, 0, [&options]()
    {
        metrics::Channel channel("Main.Editor", YAGET_METRICS_CHANNEL_FILE_LINE);

        if (options.find<bool>("vts_fix", false))
        {
            yaget::io::diag::VirtualTransportSystem vtsFixer(false, "$(DatabaseFolder)/vts.sqlite");
        }

        return 0;
    });

    return result;
}
