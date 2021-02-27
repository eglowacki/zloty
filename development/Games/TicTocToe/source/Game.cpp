#include "Game.h"
#include "BoardComponent.h"
#include "BoardSystem.h"
#include "RenderSystem.h"
#include "GameCoordinator.h"
#include "ScoreSystem.h"
#include "ScoreComponent.h"
#include "InputComponent.h"
#include "PlayerComponent.h"
#include "InventoryComponent.h"
#include "PieceComponent.h"
#include "AssetComponent.h"
#include "RenderComponent.h"
#include "VTS/ResolvedAssets.h"
#include "VTS/ToolVirtualTransportSystem.h"
#include "HashUtilities.h"
#include "App/ConsoleApplication.h"
#include "Items/ItemsDirector.h"
#include "StringHelpers.h"
#include "MemoryManager/PoolAllocator.h"

#include <concepts>

#include "GameSystem/Messaging.h"


namespace yaget::ylog
{
    yaget::Strings GetRegisteredTags()
    {
        yaget::Strings tags =
        {
            #include "Logger/LogTags.h"
            "SPAM",
            "TTT"
        };

        return tags;
    }
}

YAGET_BRAND_NAME_F("Beyond Limits")
YAGET_CUSTOMIZE_STRIP_KEYWORDS(",::ttt,ttt::,::bc,bc::,::ivc,ivc::,::pic,pic::,::pc,pc::,::ac,ac::")



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

namespace
{
    namespace internal
    {
        template<class TupType, size_t... I>
        void yaget_print(const TupType& _tup, std::index_sequence<I...>, std::string& message)
        {
            (..., (message += (I == 0 ? "" : ", ") + yaget::conv::Convertor<std::tuple_element_t<I, TupType>>::ToString(std::get<I>(_tup))));
        }

        template<class... T>
        void yaget_print(const std::tuple<T...>& _tup, std::string& message)
        {
            yaget_print(_tup, std::make_index_sequence<sizeof...(T)>(), message);
        }
    } // namespace internal

    template <typename C, typename T>
    struct Adder
    {
        Adder(C& coordinator) : mCoordinator(coordinator)
        {}

        template <typename... S>
        void operator()(S&&... params)
        {
            mCoordinator.template AddComponent<T>(params...);
        };

        C& mCoordinator;
    };

    struct PolicyName
    {
        constexpr static bool AutoComponent = true;
    };

    //template <typename T>
    //struct Adder
    //{
    //    to adder = [](auto&&... params) { coordinator.AddComponent<T>(params...); };

    //};
#if 0
    void ReadDirectorFile(yaget::Application& app, const std::string& name)
    {
        using namespace yaget;
        using Section = io::VirtualTransportSystem::Section;
        using FullRow = ttt::GameSystemsCoordinator::CoordinatorSet::FullRow;

        const Section directorSection(name);
        io::SingleBLobLoader<io::JsonAsset> directorBlobLoader(app.VTS(), directorSection);
        if (auto asset = directorBlobLoader.GetAsset())
        {
            auto& idCache = app.IdCache;
            ttt::GameSystemsCoordinator gameSystemsCoordinator;

            if (json::IsSectionValid(asset->root, "Description", ""))
            {
                const auto& itemsBlock = json::GetSection(asset->root, "Description", "Items");

                for (const auto& itemBlock: itemsBlock)
                {
                    const auto id = idCache.GetId(IdGameCache::IdType::Burnable);
                    for (const auto& componentBlock: itemBlock)
                    {
                        auto componentName = json::GetValue<std::string>(componentBlock, "Type", {});
                        YAGET_UTIL_THROW_ASSERT("TTT", !componentName.empty(), "Component Type can not be empty and must have one of game components names.");

                        if (!componentName.ends_with("Component") && PolicyName::AutoComponent)
                        {
                            componentName += "Component";
                        }

                        meta::for_each_type<FullRow>([id, &componentName, &componentBlock, &gameSystemsCoordinator]<typename T0>(const T0&)
                        {
                            const auto& tName = comp::db::GetPolicyRowNames<std::tuple<T0>>();
                            if (!tName.empty() && *tName.begin() == componentName)
                            {
                                using BaseType = meta::strip_qualifiers_t<T0>;

                                using ParameterNames = typename comp::db::RowDescription_t<BaseType>::Row;
                                using ParameterPack = typename comp::db::RowDescription_t<BaseType>::Types;

                                const auto& names = conv::Combine(comp::db::GetPolicyRowNames<ParameterNames>(), ", ");
                                const auto componentParams = json::GetValue<ParameterPack>(componentBlock, "Params", {});

                                auto idParam = std::make_tuple(id);
                                auto newRow = std::tuple_cat(idParam, componentParams);

                                //if constexpr (meta::tuple_is_element_v<T0, ttt::Entity::Row>)
                                //{
                                //    auto& coordinator = gameSystemsCoordinator.GetCoordinator<ttt::Entity>();

                                //    // this should be factorized out and templetized
                                //    if constexpr (std::is_same_v<BaseType, ttt::PlayerComponent>)
                                //    {
                                //        auto adder = [&coordinator](auto&&... params) { coordinator.AddComponent<ttt::PlayerComponent>(params...); };
                                //        std::apply(adder, newRow);
                                //    }
                                //    else if constexpr (std::is_same_v<BaseType, ttt::InputComponent>)
                                //    {
                                //        auto adder = [&coordinator](auto&&... params) { coordinator.AddComponent<ttt::InputComponent>(params...); };
                                //        std::apply(adder, newRow);
                                //    }
                                //    else if constexpr (std::is_same_v<BaseType, ttt::InventoryComponent>)
                                //    {
                                //        auto adder = [&coordinator](auto&&... params) { coordinator.AddComponent<ttt::InventoryComponent>(params...); };
                                //        std::apply(adder, newRow);
                                //    }
                                //    else if constexpr (std::is_same_v<BaseType, ttt::PieceComponent>)
                                //    {
                                //        auto adder = [&coordinator](auto&&... params) { coordinator.AddComponent<ttt::PieceComponent>(params...); };
                                //        std::apply(adder, newRow);
                                //    }
                                //}
                                //else if constexpr (meta::tuple_is_element_v<T0, ttt::GlobalEntity::Row>)
                                //{
                                //    // this should be factorized out and templetized
                                //    auto& coordinator = gameSystemsCoordinator.GetCoordinator<ttt::GlobalEntity>();

                                //    if constexpr (std::is_same_v<BaseType, ttt::BoardComponent>)
                                //    {
                                //        auto adder = [&coordinator](auto&&... params) { coordinator.AddComponent<ttt::BoardComponent>(params...); };
                                //        std::apply(adder, newRow);
                                //    }
                                //    else if constexpr (std::is_same_v<BaseType, ttt::ScoreComponent>)
                                //    {
                                //        auto adder = [&coordinator](auto&&... params) { coordinator.AddComponent<ttt::ScoreComponent>(params...); };
                                //        std::apply(adder, newRow);
                                //    }
                                //}

                                std::string message;
                                internal::yaget_print(newRow, message);
                                //const auto foo = meta::type_name<T0>();
                                const auto t0Name = meta::ViewToString(meta::type_name<T0>());
                                YLOG_NOTICE("TTT", "coordinator.AddComponent<%s>(%s)", t0Name.c_str(), message.c_str());

                                int z = 0;
                            }
                        });
                    }
                }
            }
            if (json::IsSectionValid(asset->root, "Load", ""))
            {

            }
        }
    }
#endif

}


int ttt::game::Run(yaget::args::Options& options)
{
    using namespace yaget;

    // basic initialization of console application
    const io::VirtualTransportSystem::AssetResolvers resolvers = {
        { "JSON", io::ResolveAsset<yaget::io::JsonAsset> }
    };

    const auto& vtsConfig = dev::CurrentConfiguration().mInit.mVTSConfig;
    io::tool::VirtualTransportSystemDefault vts(vtsConfig, resolvers);

    items::DefaultDirector<ttt::GameSystemsCoordinator> director(vts);
    app::DefaultConsole app("Yaget.Tic-Tac-Toe", director, vts, options);

    ttt::Messaging messaging{};
    return comp::gs::RunGame<ttt::GameSystemsCoordinator, ttt::RenderSystemsCoordinator>(messaging, app);
}

YAGET_COMPILE_WARNING_LEVEL_END
