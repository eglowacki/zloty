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


namespace yaget::testing
{
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

    //using QueryRow = std::tuple<comp::MenuComponent*, comp::InputComponent*>;

    //constexpr bool usesGlobal = comp::internalc::uses_global_coordinator<game::GameCoordinatorSet::Coordinators, QueryRow>();

    //using Policy = std::tuple_element_t<1, game::GameCoordinatorSet::Coordinators>::Policy;

    //bool isCoordinatorGlobal = comp::internalc::requires_global_coordinator<Policy>;
    //using RequestedRow = comp::tuple_get_union_t<QueryRow, Policy::Row>;

    //RequestedRow requestedRow{};
    //requestedRow;

    return comp::gs::RunGame<game::DefensorSystemsCoordinator>(messaging, app);
}
