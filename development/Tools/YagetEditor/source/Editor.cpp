#include "Editor.h"

#include "Render/DesktopApplication.h"
#include "EditorGameCoordinator.h"
#include "RenderGameCoordinator.h"
#include "EditorGameTypes.h"

#include "Items/ItemsDirector.h"

//#include "VTS/DefaultResolvers.h"
#include "VTS/ResolvedAssets.h"
#include "VTS/ToolVirtualTransportSystem.h"

//struct Entity : yaget::comp::RowPolicy<yaget::editor::EmptyComponent*, yaget::editor::EmptyComponent*>
//{};

int yaget::editor::Run(yaget::args::Options& options)
{
    const io::VirtualTransportSystem::AssetResolvers resolvers = {
        { "JSON", io::ResolveAsset<yaget::io::JsonAsset> }
    };

    const auto& vtsConfig = dev::CurrentConfiguration().mInit.mVTSConfig;
    io::tool::VirtualTransportSystemDefault vts(vtsConfig, resolvers);

    // for now we always reset Director while changes to schema
    items::DefaultDirector<EditorSystemsCoordinator> director(vts, "Director", items::Director::RuntimeMode::Reset);
    render::DesktopApplication app("Yaget.Editor", director, vts, options);
    Messaging messaging{};

    return comp::gs::RunGame<EditorSystemsCoordinator, RenderSystemsCoordinator>(messaging, app);



    //using namespace yaget;

    //// basic initialization of console application
    //const io::VirtualTransportSystem::AssetResolvers resolvers = {
    //    { "JSON", io::ResolveAsset<yaget::io::JsonAsset> }
    //};

    //const auto& vtsConfig = dev::CurrentConfiguration().mInit.mVTSConfig;
    //io::tool::VirtualTransportSystemDefault vts(vtsConfig, resolvers);

    //items::DefaultDirector<ttt::GameSystemsCoordinator> director(vts);
    //app::DefaultConsole app("Yaget.Tic-Tac-Toe", director, vts, options);

    //ttt::Messaging messaging{};
    //return comp::gs::RunGame<ttt::GameSystemsCoordinator, ttt::RenderSystemsCoordinator>(messaging, app);
    
    return 0;
}
