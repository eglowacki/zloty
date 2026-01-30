#include "MainGame.h"

#include <Debugging/DevConfiguration.h>
#include "DefensorGameTypes.h"
#include "DefensorGameCoordinator.h"
#include "DefensorRenderCoordinator.h"
#include "Items/ItemsDirector.h"
#include "Render/DesktopApplication.h"
#include "VTS/DiagnosticVirtualTransportSystem.h"
#include "VTS/ResolvedAssets.h"
#include "VTS/ToolVirtualTransportSystem.h"
#include "Render/AdapterInfo.h"
#include "../resource.h"

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

        const auto selectedAdapter = yaget::render::info::SelectDefaultAdapter(configInitBlock.ResX, configInitBlock.ResY);
        A app("Yaget.Defensor", director, vts, options, selectedAdapter);

        //return std::move(app);
        M messaging{};

        return comp::gs::RunGame<S>(messaging, app);
    }
}


namespace 
{
    yaget::io::Tag AttachTransientAsset(yaget::render::AssetCacheType assetCacheType, yaget::io::VirtualTransportSystem& vts)
    {
        using namespace yaget;

        auto sigSection = render::AssetCache::operator[](assetCacheType);
        auto tag = vts.GenerateTag(sigSection);
        std::shared_ptr<io::Asset> newAsset = io::ResolveAsset<io::BinAsset>({}, tag, vts);
        vts.AttachTransientBlob(newAsset);

        return tag;
    }
    
}


int defensor::Run(const yaget::args::Options& options)
{
    using namespace yaget;

    if (options.find<bool>("vts_fix", false))
    {
        yaget::io::diag::VirtualTransportSystem vtsFixer(false, "$(DatabaseFolder)/vts.sqlite");
    }

    const auto msgTextLine = comp::db::GenerateSystemsCoordinator<game::DefensorSystemsCoordinator, comp::db::GenerateCoordinator::Log>();
    YLOG_INFO("DEF", msgTextLine.c_str());

    const io::VirtualTransportSystem::AssetResolvers resolvers = {
        { "JSON", io::ResolveAsset<io::JsonAsset> },
        { "COMP", io::ResolveAsset<io::StringsAsset> },
        { "PERS", io::ResolveAsset<io::StringsAsset> },
        { "BIN", io::ResolveAsset<io::BinAsset> }
    };

    const auto& configInitBlock = dev::CurrentConfiguration().mInit;
    io::tool::VirtualTransportSystemDefault vts(configInitBlock.mVTSConfig, resolvers);

    // we want to preserve Director DB content between runs, with option to re-initialize db to default values.
    const items::Director::RuntimeMode directorMode = options.find<bool>("director_fix", false) ? items::Director::RuntimeMode::Reset : items::Director::RuntimeMode::Default;
    items::DefaultDirector<game::DefensorSystemsCoordinator> director("Director", directorMode);

    yaget::render::DesktopApplication::IconId = IDI_ICON2;
    const auto selectedAdapter = yaget::render::info::SelectDefaultAdapter(configInitBlock.ResX, configInitBlock.ResY);
    yaget::render::DesktopApplication app("Yaget.Defensor", director, vts, options, selectedAdapter);

    game::Messaging messaging{};

    AttachTransientAsset(yaget::render::BasicSignature, app.VTS());
    AttachTransientAsset(yaget::render::BasicPipeline, app.VTS());
    yaget::render::AssetCache::PopulateTypeToSection(io::VirtualTransportSystem::Section("Manifest@TypeToSection"), app.VTS());

    auto returnResult =  comp::gs::RunGame<game::DefensorSystemsCoordinator, render::DefensorSystemsCoordinator>(messaging, app);

    yaget::render::AssetCache::SaveTypeToSection(io::VirtualTransportSystem::Section("ManifestWrite@TypeToSection"), app.VTS());

    return returnResult;
}
