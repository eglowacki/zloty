#include "Game.h"
#include "BoardComponent.h"
#include "BoardSystem.h"
#include "GameCoordinator.h"
#include "ScoreSystem.h"
#include "ScoreComponent.h"
#include "InputComponent.h"
#include "PlayerComponent.h"
#include "InventoryComponent.h"
#include "PieceComponent.h"
#include "VTS/ResolvedAssets.h"
#include "VTS/ToolVirtualTransportSystem.h"
#include "HashUtilities.h"

#include "App/ConsoleApplication.h"

#include "Items/ItemsDirector.h"

#include "Meta/CompilerAlgo.h"
#include "Components/CoordinatorSet.h"
#include "Components/GameSystemsCoordinator.h"

#include <concepts>
//namespace std
//{
//    template <class _Ty>
//    struct is_literal_type : bool_constant<__is_literal_type(_Ty)> {
//        // determine whether _Ty is a literal type
//    };
//
//}
//
//#include <boost/hana.hpp>


namespace yaget::ylog
{
    yaget::Strings GetRegisteredTags()
    {
        yaget::Strings tags =
        {
            #include "Logger/LogTags.h"
            "SPAM"
        };

        return tags;
    }
}

YAGET_BRAND_NAME_F("Beyond Limits")

#define YAGET_CUSTOMIZE_STRIP_KEYWORDS(set) \
    YAGET_USER_STRIP_KEYWORDS_F(defaultSet) \
    { \
        using namespace yaget; \
 \
        static Initer initer(defaultSet, set); \
        return initer.mKeywords.c_str(); \
    }

YAGET_CUSTOMIZE_STRIP_KEYWORDS(",::ttt,ttt::,::bc,bc::,::ivc,ivc::,::pic,pic::,::pc,pc::")



//namespace yaget::comp::db
//{
//    template <>
//    struct CoordinatorName <ttt::GameCoordinator::GlobalCoordinator>
//    {
//        static constexpr const char* Name() { return "Globals"; }
//    };
//
//    template <>
//    struct CoordinatorName <ttt::GameCoordinator::EntityCoordinator>
//    {
//        static constexpr const char* Name() { return "Entities"; }
//    };
//
//    template <>
//    struct CoordinatorId <ttt::GameCoordinator::GlobalCoordinator>
//    {
//        static constexpr int Value() { return ttt::GameCoordinator::GLOBAL_ID; }
//    };
//
//    template <>
//    struct CoordinatorId <ttt::GameCoordinator::EntityCoordinator>
//    {
//        static constexpr int Value() { return ttt::GameCoordinator::ENTITY_ID; }
//    };
//
//}


//using GlobalEntity = yaget::comp::RowPolicy<BoardComponent*, ScoreComponent*>;
//
////! This represents allowable components that can form entity
////! For this game, we will have player consist of:
////! Input, Player, Inventory
////!
////! and entities representing pieces on a board, which are own by player inventory until placed on a board by player
////! Piece
//
//using Entity = yaget::comp::RowPolicy<InputComponent*, PlayerComponent*, InventoryComponent*, PieceComponent*>;
//
////The actual coordinator of our game which uses RowPolicy outlined above
//using GamePolicy = yaget::comp::CoordinatorPolicy<Entity, GlobalEntity>;
//
//
//using Entity = typename P::Entity;
//using Global = typename P::Global;
//using Systems = std::tuple<S...>;
//using RenderCallback = std::function<void(const time::GameClock& /*gameClock*/, metrics::Channel& /*channel*/)>;
//
//using GlobalCoordinator = comp::Coordinator<Global>;
//using EntityCoordinator = comp::Coordinator<Entity>;
//using Coordinators = std::tuple<GlobalCoordinator, EntityCoordinator>;
//
//
//using GlobalCoordinator = comp::Coordinator<GlobalEntity>;
//using EntityCoordinator = comp::Coordinator<Entity>;
//using Coordinators = std::tuple<GlobalCoordinator, EntityCoordinator>;


YAGET_COMPILE_WARNING_LEVEL_START(3, "Development of CoordinatorSet class")

int ttt::game::Run(yaget::args::Options& options)
{
    using namespace yaget;

    //auto Row12 = tuple_combine2<0, 2, GlobalCoordinator, EntityCoordinator>();


    // basic initialization of console application
    const io::VirtualTransportSystem::AssetResolvers resolvers = {
        { "JSON", io::ResolveAsset<yaget::io::JsonAsset> }
    };

    const auto& vtsConfig = dev::CurrentConfiguration().mInit.mVTSConfig;

    io::tool::VirtualTransportSystemDefault vts(vtsConfig, resolvers, "$(DatabaseFolder)/vts.sqlite");
    items::BlankDefaultDirector director;
    app::DefaultConsole app("Yaget.Tic-Tac-Toe", director, vts, options);

    // starting game initialization and setup
    ttt::GameSystemsCoordinator gameSystemsCoordinator;
    auto& globalCoordinator = gameSystemsCoordinator.GetCoordinator<ttt::GlobalEntity>();
    auto& entityCoordinator = gameSystemsCoordinator.GetCoordinator<ttt::Entity>();

    auto id = app.IdCache.GetId(IdGameCache::IdType::itBurnable);
    globalCoordinator.AddComponent<ttt::BoardComponent>(id, 3, 3);


    metrics::Channel channel(nullptr, nullptr, 0);
    time::GameClock gameClock;

    gameSystemsCoordinator.Tick(gameClock, channel);

    //app.Run(gameClock, channel);




    //GameCoordinatorSet coordinatorSet;
    //auto& globalCoordinator = coordinatorSet.GetCoordinator<ttt::GlobalEntity>();
    //auto& entityCoordinator = coordinatorSet.GetCoordinator<ttt::Entity>();

    //// add global components
    //auto id = app.IdCache.GetId(IdGameCache::IdType::itBurnable);
    //globalCoordinator.AddComponent<ttt::BoardComponent>(id, 3, 3);


    //// add regular world entities components
    //id = app.IdCache.GetId(IdGameCache::IdType::itBurnable);
    //entityCoordinator.AddComponent<ttt::InputComponent>(id);

    //id = app.IdCache.GetId(IdGameCache::IdType::itBurnable);
    //entityCoordinator.AddComponent<ttt::InputComponent>(id);

    ////coordinatorSet.ForEach<ttt::InputComponent*, ttt::PlayerComponent*, ttt::InventoryComponent*, ttt::PieceComponent*>([](ttt::InputComponent*, ttt::PlayerComponent*, ttt::InventoryComponent*, ttt::PieceComponent*)

    ////using TreeEntity = std::tuple<ttt::InputComponent*, ttt::ScoreComponent*, ttt::InventoryComponent*>;
    //using TreeEntity = std::tuple<ttt::BoardComponent*, ttt::InputComponent*>;

    //coordinatorSet.ForEach<TreeEntity>([](comp::Id_t id, auto&& param)
    //{
    //    int z = 0;

    //    return true;
    //});

    ////BoardSystem boardSystem;
    ////ScoreSystem scoreSystem;

    ////GameCoordinator gameCoordinator(&boardSystem, &scoreSystem);


    return 0;
}

YAGET_COMPILE_WARNING_LEVEL_END

//const io::VirtualTransportSystem::AssetResolvers resolvers = {
//    { "JSON", io::ResolveAsset<io::JsonAsset> }
//};

//io::tool::VirtualTransportSystemDefault vts(dev::CurrentConfiguration().mInit.mVTSConfig, resolvers, "$(DatabaseFolder)/vts.sqlite");

//const int64_t schemaVersion = Database::NonVersioned;
////const auto& pongerSchema = comp::db::GenerateGameDirectorSchema<pong::GameCoordinator>(schemaVersion);
//items::Director director("$(DatabaseFolder)/director.sqlite", {}, schemaVersion);

//app::DefaultConsole app("Yaget.Tic-Tac-Toe", director, vts, options);

//GameDirector gameDirector;
//auto logicCallback = [&gameDirector](auto&&... params) { gameDirector.GameLoop(params...); };

//const auto result = app.Run(logicCallback, {}, {}, {}, {});
//return result;
