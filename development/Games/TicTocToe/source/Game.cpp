#include "Game.h"
#include "GameCoordinator.h"
#include "BoardSystem.h"
#include "ScoreSystem.h"
#include "BoardComponent.h"
#include "ScoreComponent.h"
#include "InputComponent.h"
#include "PlayerComponent.h"
#include "InventoryComponent.h"
#include "PieceComponent.h"
#include "VTS/ResolvedAssets.h"
#include "VTS/ToolVirtualTransportSystem.h"
#include "HashUtilities.h"

#include "Items/ItemsDirector.h"


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

YAGET_USER_STRIP_KEYWORDS_F(defaultSet)
{
    using namespace yaget;

    static Initer initer(defaultSet, ",::ttt,ttt::,::bc,bc::,::ivc,ivc::,::pic,pic::,::pc,pc::");
    return initer.mKeywords.c_str();
}



namespace yaget::comp::db
{
    template <>
    struct CoordinatorName <ttt::GameCoordinator::GlobalCoordinator>
    {
        static constexpr const char* Name() { return "Globals"; }
    };

    template <>
    struct CoordinatorName <ttt::GameCoordinator::EntityCoordinator>
    {
        static constexpr const char* Name() { return "Entities"; }
    };

    template <>
    struct CoordinatorId <ttt::GameCoordinator::GlobalCoordinator>
    {
        static constexpr int Value() { return ttt::GameCoordinator::GLOBAL_ID; }
    };

    template <>
    struct CoordinatorId <ttt::GameCoordinator::EntityCoordinator>
    {
        static constexpr int Value() { return ttt::GameCoordinator::ENTITY_ID; }
    };

}


#include <concepts>

//template<typename T>
//concept has_barV = requires(const T & t)
//{
//    t.BarV;
//};
//
//
//template<typename T>
//concept has_bar = requires(const T & t)
//{
//    t.Bar();
//};
//
//struct Foo
//{
//    float BarV2 = 0;
//    void Bar() const {}
//
//    int z = 0;
//};
//
//bool r = has_bar<Foo>;
//bool r1 = has_barV<Foo>;

//
//template<typename T>
//concept HasCompRow = requires (T)
//{
//    T::Row;
//};
//
////template <typename T> requires HasCompRow<T>
////struct Foo
////{
////    using Row = typename T::Row;
////
////    Row mRow{};
////};
//
//
//template <typename T>
//auto GetPropRow()
//{
//    if constexpr (HasCompRow<T>)
//    {
//        struct Foo
//        {
//            using Row = typename T::Row;
//        };
//
//        return Foo{};
//    }
//    else
//    {
//        struct Foo
//        {
//            using Row = char;
//        };
//
//        return Foo{};
//    }
//}
//
//template <typename T>
//struct PropAlias
//{
//    using Type = typename decltype(GetPropRow<T>())::Row;
//};
//
//struct Bar
//{
//    using Row = int;
//};
//
//struct Bar2
//{
//};



YAGET_COMPILE_SUPRESS_START(4100, "'': unreferenced local variable")

int ttt::game::Run(yaget::args::Options& options)
{
    using namespace yaget;

    const io::VirtualTransportSystem::AssetResolvers resolvers = {
        { "JSON", io::ResolveAsset<yaget::io::JsonAsset> }
    };

    const auto& vtsConfig = dev::CurrentConfiguration().mInit.mVTSConfig;

    io::tool::VirtualTransportSystemDefault vts(vtsConfig, resolvers, "$(DatabaseFolder)/vts.sqlite");

    int64_t schemaVersion = Database::NonVersioned;
    const auto& pongerSchema = comp::db::GenerateGameDirectorSchema<GameCoordinator>(schemaVersion);
    items::Director director("$(DatabaseFolder)/director.sqlite", pongerSchema, schemaVersion);

    BoardSystem boardSystem;
    ScoreSystem scoreSystem;

    GameCoordinator gameCoordinator(&boardSystem, &scoreSystem);

    return 0;
}

YAGET_COMPILE_SUPRESS_END


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
