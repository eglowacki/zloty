#include "Editor.h"
#include "EditorGameCoordinator.h"
#include "EditorGameTypes.h"
#include "Items/ItemsDirector.h"
#include "Render/DesktopApplication.h"
#include "RenderGameCoordinator.h"
#include "VTS/ResolvedAssets.h"
#include "VTS/ToolVirtualTransportSystem.h"
#include "Render/AdapterInfo.h"
#include "App/Display.h"

#include <source_location>


int yaget::editor::Run(const yaget::args::Options& options)
{
    const io::VirtualTransportSystem::AssetResolvers resolvers = {
        { "JSON", io::ResolveAsset<yaget::io::JsonAsset> }
    };

    const auto& configInitBlock = dev::CurrentConfiguration().mInit;
    const auto& vtsConfig = configInitBlock.mVTSConfig;
    io::tool::VirtualTransportSystemDefault vts(vtsConfig, resolvers);

    auto filters = yaget::render::info::GetDefaultFilters();
    auto hardwareAdapters = yaget::render::info::EnumerateAdapters(filters, false /*referenceRasterizer*/);

    const size_t resX = configInitBlock.ResX;
    const size_t resY = configInitBlock.ResY;
    const bool fullScreen = configInitBlock.FullScreen;
    //const bool softwareRender = configInitBlock.SoftwareRender;

    yaget::render::info::Filters resolutionFilter{
        nullptr,
        nullptr,
        nullptr,
        nullptr,
        [resX, resY](auto resolution)
        {
            return resolution.mRefreshRate == 60 && resolution.mWidth == resX && resolution.mHeight == resY;
        }
    };

    auto selectedAdapter = yaget::render::info::SelectAdapter(hardwareAdapters, resolutionFilter);

    if (!selectedAdapter.IsValid())
    {
        app::SysDisplays displays;
        const auto& monitor = displays.FindPrimary();

        resolutionFilter.mOutput = [monitor](auto name)
        {
            return monitor.DeviceName() == name;
        };

        const auto width = monitor.Width();
        const auto height = monitor.Height();

        resolutionFilter.mResolution = [width, height](auto resolution)
        {
            return resolution.mRefreshRate == 60 && resolution.mWidth == width && resolution.mHeight == height;
        };

        selectedAdapter = yaget::render::info::SelectAdapter(hardwareAdapters, resolutionFilter);
    }

    // for now we always reset Director while changes to schema are WIP
    items::DefaultDirector<EditorSystemsCoordinator> director(vts, "Director", items::Director::RuntimeMode::Reset);
    render::DesktopApplication app("Yaget.Editor", director, vts, options, selectedAdapter);
    Messaging messaging{};

#if 0
      // this represent all component which are managed and allowed to be created
    // by this CoordinatorSet
    using Ed_Row = EditorSystemsCoordinator::CoordinatorSet::FullRow;
    Ed_Row edRow{};

    // all Systems which operate/examine some set of specific components
    using Ed_Systems = EditorSystemsCoordinator::Systems;
    Ed_Systems edSystems;

    meta::for_each_type<Ed_Systems>([]<typename T0>(const T0&)
    {
        using BaseSystem = meta::strip_qualifiers_t<T0>;

        // tuple of exact components that this BaseSystem operates/examines on
        using SystemSignature = typename BaseSystem::Row;

        const auto callSig = comp::db::GetPolicyRowNames<SystemSignature>();
        SystemSignature systemSignature{};

        int z = 0;
        z;
    });
#endif

    return comp::gs::RunGame<EditorSystemsCoordinator, RenderSystemsCoordinator>(messaging, app);
}
