#include "Editor.h"
#include "EditorGameCoordinator.h"
#include "EditorGameTypes.h"
#include "Items/ItemsDirector.h"
#include "Render/DesktopApplication.h"
#include "RenderGameCoordinator.h"
#include "VTS/ResolvedAssets.h"
#include "VTS/ToolVirtualTransportSystem.h"

#include <source_location>


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
