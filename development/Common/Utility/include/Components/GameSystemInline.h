//////////////////////////////////////////////////////////////////////
// GameSystemInline.h
//
//  Copyright 2/21/2024 Edgar Glowacki
//
//  Maintained by: Edgar
//
//  NOTES:
//		Inline implementation of GameSystem class
//		Only included by GameSystem.h file.
//
//
//  #include "Components/GameSystemInline.h"
//
//////////////////////////////////////////////////////////////////////
//! \file
#pragma once

#ifndef YAGET_GAME_SYSTEM_INCLUDE_IMPLEMENTATION
    #error "Do not include this file explicitly."
#endif // YAGET_GAME_SYSTEM_INCLUDE_IMPLEMENTATION


namespace yaget::comp::gs
{
    //---------------------------------------------------------------------------------------------------------
    template <typename CS, typename E, typename M, typename... Comps>
    void GameSystem<CS, E, M, Comps...>::Tick(const time::GameClock& gameClock, metrics::Channel& channel)
    {
        mCoordinatorSet.template ForEach<Row>([this, &gameClock, &channel](Id_t id, const auto& row)
        {
            Update(id, gameClock, channel, row);
            return true;
        });

        if constexpr (std::is_same_v<EndMarker, GenerateEndMarker>)
        {
            Update(END_ID_MARKER, gameClock, channel, {});
        }
    }


    //---------------------------------------------------------------------------------------------------------
    template <typename CS, typename E, typename M, typename... Comps>
    template <typename C>
    Coordinator<C>& GameSystem<CS, E, M, Comps...>::GetCoordinator()
    {
        return mCoordinatorSet.template GetCoordinator<C>();
    }


    //---------------------------------------------------------------------------------------------------------
    template <typename CS, typename E, typename M, typename... Comps>
    template <typename C>
    const Coordinator<C>& GameSystem<CS, E, M, Comps...>::GetCoordinator() const
    {
        return mCoordinatorSet.template GetCoordinator<C>();
    }

    namespace internalgs
    {
        template <typename TTuple,
                 typename TElement,
                 size_t Index = 0,
                 size_t Size = std::tuple_size_v<std::remove_reference_t<TTuple>>>
        constexpr size_t tuple_get_index_element()
        {
            if constexpr (Index < Size)
            {
                using BaseTupleElement = meta::strip_qualifiers_t<std::tuple_element_t<Index, TTuple>>;
                using BaseElement = meta::strip_qualifiers_t<TElement>;

                if constexpr (std::is_same_v<BaseTupleElement, BaseElement>)
                {
                    return Index;
                }
                else
                {
                    if constexpr (Index + 1 < Size)
                    {
                        return tuple_get_index_element<TTuple, TElement, Index + 1>();
                    }
                }

            }

            return -1;
        }

        template <typename TTuple, typename TElement>
        inline constexpr size_t tuple_get_index_element_v = tuple_get_index_element<TTuple, TElement>();

        template <typename TTuple, typename TElement>
        inline constexpr bool tuple_has_element_v = tuple_get_index_element<TTuple, TElement>() != -1;

    } // namespace internalgs


    //---------------------------------------------------------------------------------------------------------
    template <typename CS, typename E, typename M, typename... Comps>
    template <typename CT, typename... Args>
    CT* GameSystem<CS, E, M, Comps...>::AddComponent(Id_t id, Args&&... args)
    {
        CT *component = nullptr;

        // testing new way of extracting C (Coordinator) from CT (ComponentType)
        meta::for_each_type<typename CS::Coordinators>([&]<typename T0>(const T0*)
        {
            using Coordinator = meta::strip_qualifiers_t<T0>;

            if constexpr (internalgs::tuple_has_element_v<typename Coordinator::FullRow, CT>)
            {
                if (!component)
                {
                    auto& coordinator = GetCoordinator<typename Coordinator::Policy>();
                    component = coordinator.template AddComponent<CT>(id, std::forward<Args>(args)...);

                    //const std::size_t index = mCoordinatorSet.template GetCoordinatorIndex<meta::strip_qualifiers_t<decltype(coordinator)>>();
                    //internalgs::add_item_to_collection(index, id, mItems);
                }
            }
        });

        return component;
    }


    //---------------------------------------------------------------------------------------------------------
    template <typename CS, typename E, typename M, typename... Comps>
    template <typename C, typename CT>
    void GameSystem<CS, E, M, Comps...>::RemoveComponent(Id_t id)
    {
        auto& coordinator = GetCoordinator<C>();
        const auto moreComponents = coordinator.template RemoveComponent<CT>(id);

        if (!moreComponents)
        {
            //const std::size_t index = mCoordinatorSet.template GetCoordinatorIndex<meta::strip_qualifiers_t<decltype(coordinator)>>();
            //internalgs::remove_item_from_collection(index, id, mItems);
        }
    }


    //---------------------------------------------------------------------------------------------------------
    template <typename CS, typename E, typename M, typename... Comps>
    template <typename C>
    void GameSystem<CS, E, M, Comps...>::RemoveComponents(Id_t id)
    {
        auto& coordinator = GetCoordinator<C>();
        coordinator.RemoveComponents(id);
        
        //const std::size_t index = mCoordinatorSet.template GetCoordinatorIndex<meta::strip_qualifiers_t<decltype(coordinator)>>();
        //internalgs::remove_item_from_collection(index, id, mItems);
    }


    //---------------------------------------------------------------------------------------------------------
    template <typename CS, typename E, typename M, typename... Comps>
    GameSystem<CS, E, M, Comps...>::~GameSystem()
    {
        internalgs::remove_all_from_collection<typename CS::Coordinators>(mCoordinatorSet, mItems);
    }


    //---------------------------------------------------------------------------------------------------------
    template <typename CS, typename E, typename M, typename... Comps>
    GameSystem<CS, E, M, Comps...>::GameSystem(const char* niceName, Messaging& messaging, Application& /*app*/, UpdateFunctor updateFunctor, CS& coordinatorSet)
        : mMessaging(messaging)
        , mNiceName(niceName)
        , mUpdateFunctor(updateFunctor)
        , mCoordinatorSet(coordinatorSet)
    {}


    //---------------------------------------------------------------------------------------------------------
    template <typename CS, typename E, typename M, typename... Comps>
    void GameSystem<CS, E, M, Comps...>::Update(Id_t id, const time::GameClock& gameClock, metrics::Channel& channel, const Row& row)
    {
        // NOTE how can we convert row to new row with some of them being const modified?
        auto newRow = std::tuple_cat(std::tie(id, gameClock, channel), row);
        std::apply(mUpdateFunctor, newRow);
    }


    namespace internalgs
    {
        //---------------------------------------------------------------------------------------------------------
        //template <
        //    typename TTuple,
        //    size_t Index,
        //    size_t Size,
        //    typename TElement,
        //    typename TCollection
        //>
        //constexpr void add_to_collection(TElement&& id, TCollection& collection)
        //{
        //    if constexpr (Index < Size)
        //    {
        //        collection[Index].insert(id);

        //        if constexpr (Index + 1 < Size)
        //        {
        //            add_to_collection<TTuple, Index + 1>(id, collection);
        //        }
        //    }
        //}


        //---------------------------------------------------------------------------------------------------------
        template <
            typename TTuple,
            size_t Index,
            size_t Size,
            typename TCoordinatorSet,
            typename TCollection
        >
        constexpr void remove_all_from_collection(TCoordinatorSet&& coordinatorSet, TCollection& collection)
        {
            if constexpr (Index < Size)
            {
                auto& coordinator = coordinatorSet.template GetCoordinator<Index>();
                coordinator.RemoveItems(collection[Index]);

                if constexpr (Index + 1 < Size)
                {
                    remove_all_from_collection<TTuple, Index + 1>(coordinatorSet, collection);
                }
            }
        }


        //---------------------------------------------------------------------------------------------------------
        template <typename TElement, typename TCollection>
        constexpr void add_item_to_collection(size_t index, TElement&& id, TCollection& collection)
        {
            collection[index].insert(id);
        }


        //---------------------------------------------------------------------------------------------------------
        template <typename TElement, typename TCollection>
        constexpr void remove_item_from_collection(size_t index, TElement&& id, TCollection& collection)
        {
            collection[index].erase(id);
        }

    } // namespace internalgs

} // namespace yaget::comp::gs
