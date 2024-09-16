#include "MainGame.h"
#include "YagetVersion.h"
#include "App/AppHarness.h"
#include "VTS/DiagnosticVirtualTransportSystem.h"
#include "LoggerCpp/OutputDebug.h"
#include "LoggerCpp/OutputFile.h"
#include "MemoryManager/PoolAllocator.h"
#include "MemoryManager/NewAllocator.h"

yaget::Strings yaget::ylog::GetRegisteredTags()
{
    yaget::Strings tags =
    {
        #include "Logger/CoreLogTags.h"
        #include "Render/Logger/RenderLogTags.h"
        "DEF",
    };

    return tags;
}

YAGET_BRAND_NAME_F("Beyond Limits")
YAGET_CUSTOMIZE_STRIP_KEYWORDS(",::defensor,defensor::,::db_location,db_location::,::db_input,db_input::,::db_squadron,db_squadron::,::db_unit,db_unit::, \
                                 ::db_menu,db_menu::,::db_script,db_script::,::db_stage,db_stage::,::items,items::")

int WINAPI WinMain(HINSTANCE /*hInstance*/, HINSTANCE /*hPrevInstance*/, LPSTR lpCmdLine, int /*nCmdShow*/)
{
    YAGET_CHECKVERSION;

    using namespace yaget;

    memory::InitializeAllocations();

    args::Options options("Yaget.Defensor");

    const int result = app::helpers::Harness<ylog::OutputFile, ylog::OutputDebug>(lpCmdLine, options, nullptr, 0, [&options]()
    {
        metrics::Channel channel("Main.Defensor");

        return defensor::Run(options);
    });

    memory::ReportAllocations();

    return result;
}
