//////////////////////////////////////////////////////////////////////
// CoordinatorSet.h
//
//  Copyright 6/16/2019 Edgar Glowacki
//
//  Maintained by: Edgar
//
//  NOTES:
//      Provides collection and management of items (set of components)
//      This used to be represented by Scene. It will iterate over
//      Coordinates, build Components row for each Coordinate and fill in 
//      all fou data, then iterating over specific system and pass
//      all found components.
//      There is a concept of Global coordinator (1 component type only exist)
//      in which case, we need to propagate that 'global' component
//      to rest of the rows as we iterate over.
//
//      If we are going to pass as ref from GameCoordinator to Systems,
//      then it also may cache previous frame, tick
//
//  #include "Components/CoordinatorSet.h"
//
//////////////////////////////////////////////////////////////////////
//! \file
#pragma once

#include "Components/Coordinator.h"
#include "Items/ItemsDirector.h"
#include <functional>


namespace yaget::comp
{
    namespace internalc
    {
        template <std::size_t TupleIndex, std::size_t MaxTupleSize, typename... Tuple>
        constexpr auto coordinator_row_combine()
        {
            using Tuples = std::tuple<typename Tuple::FullRow...>;

            constexpr auto currentRow = std::get<TupleIndex>(Tuples{});
            if constexpr (TupleIndex + 1 < MaxTupleSize)
            {
                constexpr auto nextRow = coordinator_row_combine<TupleIndex + 1, MaxTupleSize, Tuple...>();
                return std::tuple_cat(currentRow, nextRow);
            }
            else
            {
                return currentRow;
            }
        }

        template<typename TupleA, typename TupleB, std::size_t TupleIndex, std::size_t MaxTupleSize>
        constexpr auto tuple_get_union()
        {
            if constexpr (TupleIndex < MaxTupleSize)
            {
                using CurrentResult = std::tuple_element_t<TupleIndex, TupleA>;

                if constexpr (yaget::meta::tuple_is_element_v<CurrentResult, TupleB>)
                {
                    constexpr auto result = std::tuple<CurrentResult>{};
                    constexpr auto nextResult = tuple_get_union<TupleA, TupleB, TupleIndex + 1, MaxTupleSize>();
                    return std::tuple_cat(result, nextResult);
                }
                else
                {
                    constexpr auto nextResult = tuple_get_union<TupleA, TupleB, TupleIndex + 1, MaxTupleSize>();
                    return nextResult;
                }
            }
            else
            {
                return std::tuple<>{};
            }
        }

        // Copy tuple from Source to Target overwriting any values in Target
        template<typename S, typename T, int N = std::tuple_size_v<std::remove_reference_t<S>>>
        constexpr void tuple_copy(const S& source, T& target)
        {
            tuple_copy_if(source, target, [](auto...){ return true; });
        }

        // Copy tuple from Source to Target overwriting any values in Target
        // if callback returns true, otherwise skip it
        template<typename S, typename T, typename C, int N = std::tuple_size_v<std::remove_reference_t<S>>>
        constexpr void tuple_copy_if(const S& source, T& target, C callback)
        {
            using ET = std::tuple_element_t<N - 1, S>;

            const auto& sourceElement = std::get<ET>(source);
            const auto& targetElement = std::get<ET>(target);
            if (callback(sourceElement, targetElement))
            {
                std::get<ET>(target) = std::get<ET>(source);
            }

            if constexpr (N - 1 > 0)
            {
                tuple_copy_if<S, T, C, N - 1>(source, target, callback);
            }
        }

        template<typename S, typename T>
        constexpr void tuple_copy_if_source(const S& source, T& target)
        {
            tuple_copy_if(source, target, [](const auto& sourceElement, const auto& /*targetElement*/)
            {
                return sourceElement != nullptr;
            });
        }

        template <typename Coordinators, size_t Num = std::tuple_size_v<Coordinators>>
        constexpr bool has_global_coordinator()
        {
            if constexpr (Num > 0)
            {
                using CoordinatorRow = typename std::tuple_element_t<Num - 1, Coordinators>::Policy;
                CoordinatorRow coordinatorRow{};
                if constexpr (comp::requires_global_coordinator<CoordinatorRow>)
                {
                    return true;
                }

                return has_global_coordinator<Coordinators, Num - 1>();
            }

            return false;
        }
    } // namespace internalc


    template<typename TupleA, typename TupleB>
    using tuple_get_union_t = decltype(internalc::tuple_get_union<TupleA, TupleB, 0, std::tuple_size_v<TupleA>>());


    namespace internalc
    {
        template <typename Coordinators, typename QueryRow, size_t Num = std::tuple_size_v<Coordinators>>
        constexpr bool uses_global_coordinator()
        {
            if constexpr (Num > 0)
            {
                constexpr std::size_t coordinatorIndex = Num - 1;

                using CoordinatorPolicy = typename std::tuple_element_t<coordinatorIndex, Coordinators>::Policy;

                if constexpr (comp::requires_global_coordinator<CoordinatorPolicy>)
                {
                    using RequestedRow = comp::tuple_get_union_t<QueryRow, typename CoordinatorPolicy::Row>;
                    
                    if constexpr (std::tuple_size_v<RequestedRow> > 0)
                    {
                        return true;
                    }
                }

                return uses_global_coordinator<Coordinators, QueryRow, Num - 1>();
            }

            return false;
        }


        template <typename T, typename CS, std::size_t Index>
        constexpr bool IsCoordinatorHasPolicy()
        {
            using CoordinatorPolicy = typename std::tuple_element_t<Index, CS>::Policy;
            using RequestedRow = tuple_get_union_t<std::tuple<T*>, typename CoordinatorPolicy::Row>;

            return std::tuple_size_v<RequestedRow> > 0;
        }

    } // namespace internalc

    template <typename... Tuple>
    struct coordinator_row_combine
    {
        using type = decltype(internalc::coordinator_row_combine<0, std::tuple_size_v<std::remove_reference_t<std::tuple<Tuple...>>>, Tuple...>());
    };

    template<typename... Tuple>
    using coordinator_row_combine_t = typename coordinator_row_combine<Tuple...>::type;


    template<typename T>
    concept is_component_persistent = requires (T t) { {t.GetStorage()}; };



    //template<char ...C>
    //requires (sizeof...(C)%2 == 0)
    //constexpr std::string
    //operator""_hex()
    template <typename... T>
    class CoordinatorSet
    {
    public:
        using Coordinators = std::tuple<T...>;
        static constexpr size_t NumCoordinators = std::tuple_size_v<std::remove_reference_t<Coordinators>>;

        using FullRow = coordinator_row_combine_t<T...>;
        //static_assert(meta::tuple_is_unique_v<FullRow>, "Duplicate element types in CoordinatorSet FullRow");
        const Strings mComponentNames = comp::db::GetPolicyRowNames<FullRow>();

        CoordinatorSet(items::Director* director)
            : mDirector(director)
        {}

        // find all rows which contain QueryRow, and call callback for each one
        // return True to keep iterating, otherwise return False to stop
        template <typename QueryRow>
        using RowCallback = std::function<bool(Id_t, QueryRow)>;

        template <typename QueryRow>
        std::size_t ForEach(RowCallback<QueryRow> callback) const
        {
            constexpr bool usesGlobal = internalc::uses_global_coordinator<Coordinators, QueryRow>();

            // we want to compare QueryRow against each Coordinator::FullRow,
            // and collect same elements into one tuple type and return that.
            // with this type we can enumerate over all rows using returned tuple.

            using Items = std::map<comp::Id_t, QueryRow>;
            Items collectedItems;
            Items collectedGlobalItem;

            //---------------------------------------------------------------------------------------------
            // collect global row if coordinator exists
            if constexpr (usesGlobal)
            {
                meta::for_loop<NumCoordinators>([this, &collectedGlobalItem]<std::size_t T0>()
                {
                    constexpr std::size_t coordinatorIndex = T0;

                    using CoordinatorPolicy = typename std::tuple_element_t<coordinatorIndex, Coordinators>::Policy;

                    if constexpr (comp::requires_global_coordinator<CoordinatorPolicy>)
                    {
                        using RequestedRow = tuple_get_union_t<QueryRow, typename CoordinatorPolicy::Row>;
                        using RequestedRowPolicy = comp::GlobalRowPolicy<RequestedRow>;
                        
                        if constexpr (std::tuple_size_v<RequestedRow> > 0)
                        {
                            auto& coordinator = GetCoordinator<coordinatorIndex>();
                            const std::size_t numItems = coordinator.template ForEach<RequestedRowPolicy>([&collectedGlobalItem](comp::Id_t id, const auto& row)
                            {
                                internalc::tuple_copy(row, collectedGlobalItem[id]);
                                return true;
                            });
                        }
                    }
                });
            }

            bool itemsProcessed = false;
            //---------------------------------------------------------------------------------------------
            // we need to iterate over each coordinator and collect results
            meta::for_loop<NumCoordinators>([this, &collectedItems, &itemsProcessed]<std::size_t T0>()
            {
                constexpr std::size_t coordinatorIndex = T0;

                using CoordinatorPolicy = typename std::tuple_element_t<coordinatorIndex, Coordinators>::Policy;
                if constexpr (!comp::requires_global_coordinator<CoordinatorPolicy>)
                {
                    using RequestedRow = tuple_get_union_t<QueryRow, typename CoordinatorPolicy::Row>;

                    if constexpr (std::tuple_size_v<RequestedRow> > 0)
                    {
                        using RequestedRowPolicy = comp::RowPolicy<RequestedRow>;

                        auto& coordinator = GetCoordinator<coordinatorIndex>();
                        const std::size_t numItems = coordinator.template ForEach<RequestedRowPolicy>([&collectedItems](comp::Id_t id, const auto& row)
                        {
                            internalc::tuple_copy(row, collectedItems[id]);
                            return true;
                        });

                        itemsProcessed = true;
                    }
                }
            });

            //---------------------------------------------------------------------------------------------
            // iterate over collected rows and trigger callback
            std::size_t numProcessedElements = 0;

            if constexpr (usesGlobal)
            {
                // only if there is no regular items (collectedItems) and we have ONE global item (collectedGlobalItem)
                if (!itemsProcessed && collectedItems.empty() && !collectedGlobalItem.empty() && collectedGlobalItem.size() == 1)
                {
                    collectedItems = collectedGlobalItem;
                }
            }

            for (auto& [id, element] : collectedItems)
            {
                auto elementRow = element;
                if constexpr (usesGlobal)
                {
                    const auto& globalElement = collectedGlobalItem.begin();
                    // if we do have global elements copy them into parameter before triggering callback
                    internalc::tuple_copy_if_source(globalElement->second, elementRow);
                }

                if (callback(id, elementRow))
                {
                    ++numProcessedElements;
                }
                else
                {
                    break;
                }
            }

            return numProcessedElements;
        }

        template <typename C>
        comp::Coordinator<C>& GetCoordinator()
        {
            return std::get<comp::Coordinator<C>>(mCoordinators);
        }

        template <typename C>
        const comp::Coordinator<C>& GetCoordinator() const
        {
            return std::get<comp::Coordinator<C>>(mCoordinators);
        }

        template <std::size_t Index>
        auto& GetCoordinator()
        {
            return std::get<Index>(mCoordinators);
        }

        template <std::size_t Index>
        const auto& GetCoordinator() const
        {
            return std::get<Index>(mCoordinators);
        }


        //-------------------------------------------------------------------------------------------------
        // Public interface to manage items/components
        //-------------------------------------------------------------------------------------------------
        // Create and add new component with id and initialize with args... parameters.
        template <typename C, typename... Args>
        C* AddComponent(comp::Id_t id, Args&&... args)
        {
            C* component{};

            bool coordinatorFound = false;
            meta::for_loop<NumCoordinators>([&]<std::size_t T0>()
            {
                constexpr std::size_t coordinatorIndex = T0;

                if (!coordinatorFound)
                {
                    if constexpr (internalc::IsCoordinatorHasPolicy<C, Coordinators, coordinatorIndex>())
                    {
                        auto& coordinator = GetCoordinator<coordinatorIndex>();
                        component = coordinator.template AddComponent<C>(id, std::forward<Args>(args)...);

                        coordinatorFound = true;
                    }
                }
            });

            // NOTE EG: 2/23/2025
            // Could not get this code below working, so resorted to manual code above
            //FindMatchCoordinator<C>([&](auto* coordinator)
            //{
            //    /*component =*/ coordinator->template AddComponent<C>(id, std::forward<Args>(args)...);
            //    int z = 0;
            //    z;
            //});

            return component;
        }

        //-------------------------------------------------------------------------------------------------
        // Load id component data (args...) from DB, create new component with that id
        // and initialize with args... parameters. If component already exist,
        // then update the parameters of current one.
        template <typename C>
        C* LoadComponent(comp::Id_t id)
        {
            error_handlers::ThrowOnCheck(mDirector, std::format("Can not load component, director is nullptr"));

            C* component{};

            if constexpr (is_component_persistent<C>)
            {
                bool result = false;
                auto parameters = mDirector->LoadComponentState<C>(id, &result);;

                if (result)
                {
                    FindMatchCoordinator<C>([&parameters, &component, id](auto* coordinator)
                    {
                        if (component = coordinator->template FindComponent<C>(id); component)
                        {
                            component->SetStorage(parameters);
                        }
                    });

                    if (!component)
                    {
                        component = std::apply([this, id](auto &&... args)
                        {
                            return AddComponent<C>(id, args...);
                        }, parameters);
                    }
                }
            }

            return component;
        }

        //-------------------------------------------------------------------------------------------------
        template <typename C>
        bool SaveComponent(const C* component)
        {
            error_handlers::ThrowOnCheck(mDirector, "Can not save component, director is nullptr");
            const auto result = mDirector->SaveComponentState(component);
            return result;
        }

        //-------------------------------------------------------------------------------------------------
        template <typename C>
        C* FindComponent(comp::Id_t id) const
        {
            C* component{};

            FindMatchCoordinator<C>([&component, id](auto* coordinator)
            {
                component = coordinator->template FindComponent<C>(id);
            });

            return component;
        }

        //-------------------------------------------------------------------------------------------------
        template <typename C>
        bool RemoveComponent(comp::Id_t id)
        {
            bool result = false;
            bool coordinatorFound = false;

            meta::for_loop<NumCoordinators>([&]<std::size_t T0>()
            {
                constexpr std::size_t coordinatorIndex = T0;

                if (!coordinatorFound)
                {
                    if constexpr (internalc::IsCoordinatorHasPolicy<C, Coordinators, coordinatorIndex>())
                    {
                        auto& coordinator = GetCoordinator<coordinatorIndex>();

                        if (auto component = coordinator.template FindComponent<C>(id))
                        {
                            result = coordinator.RemoveComponent(id, component);
                        }

                        coordinatorFound = true;
                    }
                }
            });
            // NOTE EG: 2/23/2025
            // Could not get this code below working, so resorted to manual code above
            //FindMatchCoordinator<C>([&result, id](auto* coordinator)
            //{
            //    result = coordinator->template RemoveComponent<C>(id);
            //});

            return result;
        }

        //-------------------------------------------------------------------------------------------------
        template <typename TT = FullRow>
        TT LoadItem(comp::Id_t id)
        {
            TT item{};

            meta::for_each_type<TT>([this, id, &item]<typename T0>(const T0&)
            {
                using BaseType = meta::strip_qualifiers_t<T0>;
                std::get<T0>(item) = LoadComponent<BaseType>(id);
            });

            return item;
        }

        //-------------------------------------------------------------------------------------------------
        template <typename TT = FullRow>
        bool RemoveItem(comp::Id_t id)
        {
            bool result = true;
            meta::for_each_type<TT>([this, &result, id]<typename T0>(const T0&)
            {
                using BaseType = meta::strip_qualifiers_t<T0>;
                RemoveComponent<BaseType>(id);
            });

            return result;
        }

    private:
        //-------------------------------------------------------------------------------------------------
        template <typename C>
        constexpr void FindMatchCoordinator(auto callback) const
        {
            bool coordinatorFound = false;

            meta::for_loop<NumCoordinators>([&]<std::size_t T0>()
            {
                constexpr std::size_t coordinatorIndex = T0;

                if (!coordinatorFound)
                {
                    if constexpr (internalc::IsCoordinatorHasPolicy<C, Coordinators, coordinatorIndex>())
                    {
                        auto& coordinator = GetCoordinator<coordinatorIndex>();
                        callback(&coordinator);

                        coordinatorFound = true;
                    }
                }
            });
        }
        
        Coordinators mCoordinators;
        items::Director* mDirector{};
    };


    //----------------------------------------------------------------------------------------------------------------------------------
    template<typename T>
    concept is_systems_coordinator = requires { typename T::CoordinatorSet; };


    //----------------------------------------------------------------------------------------------------------------------------------
    template<typename T>
    concept is_coordinator_set = requires
    {
        typename T::Coordinators;
        typename T::NumCoordinators; 
    };


    //----------------------------------------------------------------------------------------------------------------------------------
    template<typename T>
    struct coordinator_set;


    //----------------------------------------------------------------------------------------------------------------------------------
    template<is_systems_coordinator T>
    struct coordinator_set<T>
    {
        using type = typename T::CoordinatorSet;
    };


    //----------------------------------------------------------------------------------------------------------------------------------
    template<is_coordinator_set T>
    struct coordinator_set<T>
    {
        using type = T;
    };


    //----------------------------------------------------------------------------------------------------------------------------------
    template<typename T>
    using coordinator_set_t = typename coordinator_set<T>::type;


    //----------------------------------------------------------------------------------------------------------------------------------
    constexpr size_t InvalidCoordinatorIndex = std::numeric_limits<size_t>::max();


    //----------------------------------------------------------------------------------------------------------------------------------
    template <typename ComponentType, typename T, typename CS = coordinator_set_t<T>>
    consteval size_t FindCoordinatorIndex()
    {
        constexpr size_t NumCoordinators = CS::NumCoordinators;
        size_t indexResult = InvalidCoordinatorIndex;

        meta::for_loop<NumCoordinators>([&indexResult]<std::size_t T0>()
        {
            if (indexResult == InvalidCoordinatorIndex)
            {
                constexpr std::size_t CoordinatorIndex = T0;

                using Coordinators = typename CS::Coordinators;
                using CoordinatorType = std::tuple_element_t<CoordinatorIndex, Coordinators>;
                using CoordinatorComponents = CoordinatorType::FullRow;

                if constexpr (yaget::meta::tuple_is_element_v<ComponentType*, CoordinatorComponents>)
                {
                    indexResult = CoordinatorIndex;
                }
            }
        });

        return indexResult;
    }


    //----------------------------------------------------------------------------------------------------------------------------------
    template <typename ComponentType, typename T>
    struct CoordinatorIndex
    {
        static constexpr size_t value = FindCoordinatorIndex<ComponentType, T>();
    };


    //----------------------------------------------------------------------------------------------------------------------------------
    template <typename ComponentType, typename T>
    constexpr size_t CoordinatorIndex_v =  CoordinatorIndex<ComponentType, T>::value;
    

    //----------------------------------------------------------------------------------------------------------------------------------
    template <typename ComponentType, typename T, typename CS =  coordinator_set_t<T>>
    consteval bool IsComponentValid()
    {
        constexpr size_t coordinatorIndex = CoordinatorIndex_v<ComponentType, CS>;
        return coordinatorIndex != InvalidCoordinatorIndex;
    }


    //----------------------------------------------------------------------------------------------------------------------------------
    enum class ComponentQueryType
    {
        Invalid,
        Valid,
        Global
    };


    //----------------------------------------------------------------------------------------------------------------------------------
    template <typename ComponentType, typename T, typename CS =  coordinator_set_t<T>>
    consteval ComponentQueryType IsComponentGlobal()
    {
        constexpr size_t coordinatorIndex = CoordinatorIndex_v<ComponentType, CS>;
        if (coordinatorIndex != InvalidCoordinatorIndex)
        {
            using Coordinators = typename CS::Coordinators;
            using CoordinatorType = std::tuple_element_t<coordinatorIndex, Coordinators>;
            using Policy = CoordinatorType::Policy;
            if constexpr (comp::requires_global_coordinator<Policy>)
            {
                return ComponentQueryType::Global;
            }

            return ComponentQueryType::Valid;
        }

        return ComponentQueryType::Invalid;
    }


    //----------------------------------------------------------------------------------------------------------------------------------
    template <typename ComponentType, typename T, typename CS =  coordinator_set_t<T>>
    struct CoordinatorType
    {
        using Coordinators = typename CS::Coordinators;
        using type = std::tuple_element_t<CoordinatorIndex_v<ComponentType, CS>, Coordinators>;
    };


    //----------------------------------------------------------------------------------------------------------------------------------
    template<typename ComponentType, typename T, typename CS =  coordinator_set_t<T>>
    using CoordinatorType_t = typename CoordinatorType<ComponentType, CS>::type;


    //----------------------------------------------------------------------------------------------------------------------------------
    //template<typename ComponentType, typename T>
    //auto GetCoordinator(T& t)
    //{
    //    t;
    //    if constexpr (coordinator_set_v<T>)
    //    {
    //        return 1;
    //        //return t.GetCoordinator<CoordinatorIndex_v<ComponentType, T>>();
    //    }
    //    else if constexpr (systems_coordinator_v<T>)
    //    {
    //        return 2;
    //        //return t.GetCoordinatorSet().GetCoordinator<CoordinatorIndex_v<ComponentType, T>>();
    //    }
    //}

    ////----------------------------------------------------------------------------------------------------------------------------------
    //template<typename ComponentType, typename T>
    //const auto& GetCoordinator(const T& t)
    //{
    //    if constexpr (is_coordinator_set<T>)
    //    {
    //        return t.GetCoordinator<CoordinatorIndex_v<ComponentType, T>>();
    //    }
    //    else if constexpr (is_systems_coordinator<T>)
    //    {
    //        return t.GetCoordinatorSet().GetCoordinator<CoordinatorIndex_v<ComponentType, T>>();
    //    }
    //}


}
