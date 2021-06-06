// yarena.cpp : Defines the entry point for the application.
//

#include "AppCoordinator.h"
#include "Ponger/PongerTypes.h"
#include "VTS/ToolVirtualTransportSystem.h"
#include "DesktopApplication.h"
#include "App/AppHarness.h"
#include "Components/GameSystem.h"
#include "Components/GameCoordinator.h"
#include "Components/Collectors.h"
#include "LoggerCpp/OutputConsole.h"
#include "LoggerCpp/OutputFile.h"
#include "LoggerCpp/OutputDebug.h"
#include "YagetVersion.h"
#include "TextureResource.h"
#include "Resources/DescriptionResource.h"
#include "Items/ItemsDirector.h"
#include "VTS/DiagnosticVirtualTransportSystem.h"
#include "STLHelper.h"
#include "Device.h"
#include "GameHarness.h"
#include "Ponger/PongerRenderer.h"
#include "Ponger/GameWorldSystem.h"
#include "PlatformDisplayModes.h"
#include "VTS/DefaultResolvers.h"
#include "Scripting/ResolvedAssets.h"
#include "Components/GameCoordinatorGenerator.h"

#include "Gui/imgui_OutputConsole.h"


//#include <kaguya/kaguya.hpp>
//kaguya::State state;
//kaguya::LuaFunction f2 = state.loadstring("a = 'test'");
//f2();
//std::string afterValue = state["a"];

//#define SOL_ALL_SAFETIES_ON 1
//#include <sol/sol.hpp>
//std::cout << "=== opening a state ===" << std::endl;
//sol::state lua;
//// open some common libraries
//lua.open_libraries(sol::lib::base, sol::lib::package);
//lua.script("print('bark bark bark!')");
//std::cout << std::endl;


#pragma message(YAGET_COMPILER_INFO)
YAGET_COMPILE_SUPPRESS_START(4100, "'': unreferenced local variable")
YAGET_COMPILE_SUPPRESS_START(4189, "'': local variable is initialized but not referenced")

namespace yaget::ylog
{
    yaget::Strings GetRegisteredTags()
    {
        yaget::Strings tags =
        {
            #include "Logger/LogTags.h"
            #include "Logger/RenderLogTags.h"
            "SCRT",
            "PYTH",
            "VTSD",
            "PONG",
            "DEVI"
        };

        return tags;
    }

} //namespace yaget::ylog


namespace pong
{
    //using GlobalEntity = comp::RowPolicy<comp::PhysicsWorldComponent*, scripting::PythonComponent*>;
    //using Entity = comp::RowPolicy<comp::LocationComponent*, comp::PhysicsComponent*, ponger::DebugComponent*, comp::ScriptComponent*, scripting::PythonComponent*>;

    using GameCoordinator = yaget::GameCoordinator<ponger::GamePolicy, ponger::GameDirectorSystem*, ponger::WorldUpdateSystem*, ponger::PythonSystem*, ponger::ColorizerSystem*, ponger::RendererGatherSystem*>;
    
}

namespace yaget::comp::db
{
    template <>
    struct CoordinatorName<pong::GameCoordinator::GlobalCoordinator>
    {
        static const char* Name() {return "Globals";}
    };

    template <>
    struct CoordinatorName<pong::GameCoordinator::EntityCoordinator>
    {
        static const char* Name() {return "Entities";}
    };


    //template <>
    //struct ComponentProperties<comp::LocationComponent>
    //{
    //    using Row = std::tuple<comp::db::Position, comp::db::Orientation, comp::db::Scale>;
    //};

    //template <>
    //struct ComponentProperties<comp::PhysicsComponent>
    //{
    //    using Row = std::tuple<>;
    //};

    //template <>
    //struct ComponentProperties<comp::PhysicsWorldComponent>
    //{
    //    using Row = std::tuple<>;
    //};

    //template <>
    //struct ComponentProperties<scripting::PythonComponent>
    //{
    //    using Row = std::tuple<scripting::db::Scripts>;
    //};

    //template <>
    //struct ComponentProperties<ponger::DebugComponent>
    //{
    //    using Row = std::tuple<ponger::db::VisualAsset>;
    //};

    //template <>
    //struct ComponentProperties<comp::ScriptComponent>
    //{
    //    using Row = std::tuple<>;
    //};

}




namespace pong
{
    using namespace yaget;

    class PongerGameHarness : public GameHarness
    {
    public:
        PongerGameHarness(IGameCoordinator& gameCoordinator, Application& app, render::Device& device) 
            : GameHarness(gameCoordinator, app, device)
            , mProcessors({ [this]() { onInitialize();  } })
        {}

    private:
        void onGameLoop(Application& /*app*/, const time::GameClock& /*gameClock*/, metrics::Channel& /*channel*/) override
        {
            while (!mProcessors.empty())
            {
                mProcessors.top()();
                mProcessors.pop();
            }
        }

        void onInitialize()
        {
            int z = 0;
            z;

        }

        using Processor = std::function<void()>;
        std::stack<Processor> mProcessors;
        //virtual void onIdle() {}
    };

    int memCounter = 0;
} // namespace pong


//void* operator new  (std::size_t count)
//{
//    pong::memCounter++;
//    void* ptr = std::malloc(count);
//    if (ptr)
//    {
//        return ptr;
//    }
//    else
//    {
//        throw std::bad_alloc{};
//    }
//}


int APIENTRY WinMain(HINSTANCE /*hInstance*/, HINSTANCE /*hPrevInstance*/, LPSTR lpCmdLine, int /*nCmdShow*/)
{
    YAGET_CHECKVERSION;
    using namespace yaget;

    //_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);

    args::Options options("Yaget.Coordinator", "Ongoing development of yaget Coordinator");
    const std::string lineCommands = std::string(lpCmdLine) + " --configuration_path=../../../Research/Coordinator/Data";

    int result = app::helpers::Harness<ylog::OutputConsole, ylog::OutputFile, ylog::OutputDebug, imgui::OutputConsole>(lineCommands.c_str(), options, [&options]()
    {
        if (options.find<bool>("vts_fix", false))
        {
            yaget::io::diag::VirtualTransportSystem vtsFixer(false, "$(DatabaseFolder)/vts.sqlite");
        }

        using Section = io::tool::VirtualTransportSystemDefault::Section;

        io::tool::AddResolvers({ { "PYTHON", &io::ResolveAsset<io::scripting::PythonAsset> } });

        io::tool::VirtualTransportSystemDefault vts(dev::CurrentConfiguration().mInit.mVTSConfig, io::tool::GetResolvers(), "$(DatabaseFolder)/vts.sqlite");

        int64_t schemaVersion = Database::NonVersioned;
        const auto& pongerSchema = comp::db::GenerateGameDirectorSchema<pong::GameCoordinator>(schemaVersion);

        items::Director director("$(DatabaseFolder)/items.sqlite", pongerSchema, schemaVersion);
        render::DesktopApplication app("Yaget.Coordinator", director, vts, options, io::tool::GetTagResolvers());

        Section rtectangleSection("Geometry@Predefined/Rectangle.pak");
        if (!vts.GetTag(rtectangleSection).IsValid())
        {
            render::GeometryConvertor::Verticies vertices{
                { -1.0f, -1.0f, 0.0f },
                { -1.0f,  1.0f, 0.0f },
                {  1.0f,  1.0f, 0.0f },
                {  1.0f, -1.0f, 0.0f }
            };

            render::GeometryConvertor::Verticies uvs{
                { 0.0f, 1.0f, 0.0f },
                { 0.0f, 0.0f, 0.0f },
                { 1.0f, 0.0f, 0.0f },
                { 1.0f, 1.0f, 0.0f }
            };

            render::GeometryConvertor::Indicies indices{ 0, 1, 2, 0, 2, 3 };

            std::transform(vertices.begin(), vertices.end(), vertices.begin(), [](const math3d::Vector3& pos)
            {
                return pos * 0.25f;
            });

            io::Buffer buffer = io::ConvertToPak(render::PakConverter(std::move(vertices), std::move(uvs), {}, std::move(indices)));
            io::Tag rectangleAssetTag = vts.GenerateTag(rtectangleSection);

            auto quadAsset = std::make_shared<io::render::PakAsset>(rectangleAssetTag, buffer, vts);
            vts.AttachBlob(quadAsset);
        }

        //-------------------------------------------------------------------------------------------------
        // research into printing db schema from GameCoordinator
        



        //-------------------------------------------------------------------------------------------------


        render::platform::Resolutions resolutions = render::platform::GetResolutions();

        // Create core/game systems
        ponger::WorldUpdateSystem worldUpdateSystem("WorldSystem", ponger::WorldUpdateSystemImpl{});
        ponger::ColorizerSystem colorizerSystem("ColorizerSystem", ponger::ColorizerSystemImpl{});

        ponger::ModuleHolder moduleHolder = std::make_shared<ponger::ContextHolder>();
        ponger::PythonSystem pythonSystem(moduleHolder);

        const auto gameDirectorScript = Section(util::ExpendEnv(dev::CurrentConfiguration().mInit.mGameDirectorScript, nullptr));
        ponger::GameDirectorSystem gameDirectorSystem(moduleHolder, app.Director(), app.IdCache, app.VTS(), app.Watcher(), gameDirectorScript);

        // setup stager, which will transfer game data to render
        ponger::PongerRenderer renderCollector(app.GetDevice());

        // Create gatherer of item locations, update stager to let renderer process them
        ponger::RendererGatherSystem locationGatherSystem("LocationGather", [&renderCollector](auto&&... params) { renderCollector.GatherLocation(params...); });

        auto renderCallback = [&renderCollector](auto&&... params) { renderCollector.Render(params...); };

        // Create game and harness and start the game
        pong::GameCoordinator gameCoordinator(renderCallback, &gameDirectorSystem , &worldUpdateSystem, &pythonSystem , &colorizerSystem, &locationGatherSystem);

        pong::PongerGameHarness gameHarness(gameCoordinator, app, app.GetDevice());
        gameHarness.Run();

        return 0;
    });

    //_CrtDumpMemoryLeaks();
    return result;
}

YAGET_COMPILE_SUPPRESS_END
YAGET_COMPILE_SUPPRESS_END
