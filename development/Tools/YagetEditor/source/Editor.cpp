#include "Editor.h"

#include "EditorGameCoordinator.h"
#include "EditorGameTypes.h"
#include "Items/ItemsDirector.h"
#include "Render/DesktopApplication.h"
#include "RenderGameCoordinator.h"
#include "VTS/ResolvedAssets.h"
#include "VTS/ToolVirtualTransportSystem.h"


int yaget::editor::Run(yaget::args::Options& options)
{
    const io::VirtualTransportSystem::AssetResolvers resolvers = {
        { "JSON", io::ResolveAsset<yaget::io::JsonAsset> }
    };

    const auto& vtsConfig = dev::CurrentConfiguration().mInit.mVTSConfig;
    io::tool::VirtualTransportSystemDefault vts(vtsConfig, resolvers);

    // for now we always reset Director while changes to schema are WIP
    items::DefaultDirector<EditorSystemsCoordinator> director(vts, "Director", items::Director::RuntimeMode::Reset);
    render::DesktopApplication app("Yaget.Editor", director, vts, options);
    Messaging messaging{};

    return comp::gs::RunGame<EditorSystemsCoordinator, RenderSystemsCoordinator>(messaging, app);
}
