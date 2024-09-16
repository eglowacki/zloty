#include "MainGame.h"

//#include "StringHelpers.h"
#include <Debugging/DevConfiguration.h>
#include "DefensorGameTypes.h"
#include "DefensorGameCoordinator.h"
#include "Items/ItemsDirector.h"
#include "Render/DesktopApplication.h"
//#include "RenderGameCoordinator.h"
#include "VTS/DiagnosticVirtualTransportSystem.h"
#include "VTS/ResolvedAssets.h"
#include "VTS/ToolVirtualTransportSystem.h"
#include "Render/AdapterInfo.h"
#include "App/Display.h"
#include "MathFacade.h"
//#include <source_location>


namespace yaget::app
{
   
    //template <typename VTS, typename D, typename A>
    //using ApplicationFramework = std::tuple<VTS, D, A>;

    template <typename VTS, typename D, typename S, typename A, typename M>
    int SetupApplicationFramework(const yaget::args::Options& options)
    {
        const io::VirtualTransportSystem::AssetResolvers resolvers = {
            { "JSON", io::ResolveAsset<io::JsonAsset> }
        };

        const auto& configInitBlock = dev::CurrentConfiguration().mInit;
        VTS vts(configInitBlock.mVTSConfig, resolvers);
        
        // we want to preserve Director DB content between runs, with option to re-initialize db to default values.
        const items::Director::RuntimeMode directorMode = options.find<bool>("director_fix", false) ? items::Director::RuntimeMode::Reset : items::Director::RuntimeMode::Default;
        D director(vts, "Director", directorMode);

        const auto selectedAdapter = render::info::SelectDefaultAdapter(configInitBlock.ResX, configInitBlock.ResY);
        A app("Yaget.Defensor", director, vts, options, selectedAdapter);

        //return std::move(app);
        M messaging{};

        return comp::gs::RunGame<S>(messaging, app);
    }
}


int defensor::Run(const yaget::args::Options& options)
{
    using namespace yaget;

    if (options.find<bool>("vts_fix", false))
    {
        yaget::io::diag::VirtualTransportSystem vtsFixer(false, "$(DatabaseFolder)/vts.sqlite");
    }

    const io::VirtualTransportSystem::AssetResolvers resolvers = {
        { "JSON", io::ResolveAsset<io::JsonAsset> }
    };

    const auto& configInitBlock = dev::CurrentConfiguration().mInit;
    io::tool::VirtualTransportSystemDefault vts(configInitBlock.mVTSConfig, resolvers);

    // we want to preserve Director DB content between runs, with option to re-initialize db to default values.
    const items::Director::RuntimeMode directorMode = options.find<bool>("director_fix", false) ? items::Director::RuntimeMode::Reset : items::Director::RuntimeMode::Default;
    items::DefaultDirector<game::DefensorSystemsCoordinator> director(vts, "Director", directorMode);

    const auto selectedAdapter = render::info::SelectDefaultAdapter(configInitBlock.ResX, configInitBlock.ResY);
    render::DesktopApplication app("Yaget.Defensor", director, vts, options, selectedAdapter);

    game::Messaging messaging{};

    return comp::gs::RunGame<game::DefensorSystemsCoordinator>(messaging, app);
}
