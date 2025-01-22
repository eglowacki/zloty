#include "Editor.h"

#include "YagetVersion.h"
#include "App/AppHarness.h"
#include "VTS/DiagnosticVirtualTransportSystem.h"
#include "LoggerCpp/OutputDebug.h"
#include "LoggerCpp/OutputFile.h"

#include "MemoryManager/PoolAllocator.h"
#include "MemoryManager/NewAllocator.h"

#include "Metrics/Concurrency.h"

yaget::Strings yaget::ylog::GetRegisteredTags()
{
    yaget::Strings tags =
    {
        #include "Logger/CoreLogTags.h"
        #include "Render/Logger/RenderLogTags.h"
        "EDIT",
    };

    return tags;
}

YAGET_BRAND_NAME_F("Beyond Limits")

int WINAPI WinMain(HINSTANCE /*hInstance*/, HINSTANCE /*hPrevInstance*/, LPSTR lpCmdLine, int /*nCmdShow*/)
{
    YAGET_CHECKVERSION;

    using namespace yaget;

    memory::InitializeAllocations();

    args::Options options("Yaget.Editor", "Yaget Editor.");

    const int result = app::helpers::Harness<ylog::OutputFile, ylog::OutputDebug>(lpCmdLine, options, nullptr, 0, [&options]()
    {
        metrics::Channel channel("Main.Editor");

        if (options.find<bool>("vts_fix", false))
        {
            yaget::io::diag::VirtualTransportSystem vtsFixer(false, "$(DatabaseFolder)/vts.sqlite");
        }

        return editor::Run(options);
    });

    memory::ReportAllocations();

    return result;
}
