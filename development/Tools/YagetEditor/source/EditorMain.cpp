#include "Editor.h"

#include "YagetVersion.h"
#include "App/AppHarness.h"
#include "VTS/DiagnosticVirtualTransportSystem.h"
#include "LoggerCpp/OutputDebug.h"
#include "LoggerCpp/OutputFile.h"
#include "Metrics/Concurrency.h"

yaget::Strings yaget::ylog::GetRegisteredTags()
{
    yaget::Strings tags =
    {
        #include "Logger/LogTags.h"
        #include "Render/Logger/RenderLogTags.h"
        "SPAM",
        "EDIT"
    };

    return tags;
}

YAGET_BRAND_NAME_F("Beyond Limits")
YAGET_CUSTOMIZE_STRIP_KEYWORDS(",::editor,editor::")


int APIENTRY WinMain(HINSTANCE /*hInstance*/, HINSTANCE /*hPrevInstance*/, LPSTR lpCmdLine, int /*nCmdShow*/)
{
    YAGET_CHECKVERSION;

    using namespace yaget;

    args::Options options("Yaget.Editor", "Yaget Editor.");

    const int result = app::helpers::Harness<ylog::OutputFile, ylog::OutputDebug>(lpCmdLine, options, nullptr, 0, [&options]()
    {
        metrics::Channel channel("Main.Editor", YAGET_METRICS_CHANNEL_FILE_LINE);

        if (options.find<bool>("vts_fix", false))
        {
            yaget::io::diag::VirtualTransportSystem vtsFixer(false, "$(DatabaseFolder)/vts.sqlite");
        }

        return editor::Run(options);
    });

    return result;
}
