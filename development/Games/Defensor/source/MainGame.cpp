#include "MainGame.h"

//#include "StringHelpers.h"
#include <Debugging/DevConfiguration.h>
#include "DefensorGameTypes.h"
#include "DefensorGameCoordinator.h"
#include "Items/ItemsDirector.h"
#include "Render/DesktopApplication.h"
//#include "RenderGameCoordinator.h"
#include "VTS/ResolvedAssets.h"
#include "VTS/ToolVirtualTransportSystem.h"
#include "Render/AdapterInfo.h"
#include "App/Display.h"
#include "MathFacade.h"
//#include <source_location>

namespace yaget::render::info
{
    Adapter SelectDefaultAdapter(size_t configInitBlock_ResX, size_t configInitBlock_ResY)
    {
        auto filters = render::info::GetDefaultFilters();
        auto hardwareAdapters = render::info::EnumerateAdapters(filters, false /*referenceRasterizer*/);

        const size_t resX = configInitBlock_ResX;
        const size_t resY = configInitBlock_ResY;
        //const bool fullScreen = configInitBlock.FullScreen;

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

        auto selectedAdapter = render::info::SelectAdapter(hardwareAdapters, resolutionFilter);

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

            selectedAdapter = render::info::SelectAdapter(hardwareAdapters, resolutionFilter);
            error_handlers::ThrowOnError(selectedAdapter.IsValid(), "Could not create default video adapter");
        }

        return selectedAdapter;
    }
    
}


int defensor::Run(const yaget::args::Options& options)
{
    using namespace yaget;

    const io::VirtualTransportSystem::AssetResolvers resolvers = {
        { "JSON", io::ResolveAsset<io::JsonAsset> }
    };

    const auto& configInitBlock = dev::CurrentConfiguration().mInit;
    io::tool::VirtualTransportSystemDefault vts(configInitBlock.mVTSConfig, resolvers);

    // for now we always reset Director while changes to schema are WIP
    //items::BlankDefaultDirector director(vts, "Director", items::Director::RuntimeMode::Reset);
    items::DefaultDirector<game::DefensorSystemsCoordinator> director(vts, "Director", items::Director::RuntimeMode::Reset);

    const auto selectedAdapter = render::info::SelectDefaultAdapter(configInitBlock.ResX, configInitBlock.ResY);
    render::DesktopApplication app("Yaget.Defensor", director, vts, options, selectedAdapter);

    game::Messaging messaging{};

    return comp::gs::RunGame<game::DefensorSystemsCoordinator>(messaging, app);
}
