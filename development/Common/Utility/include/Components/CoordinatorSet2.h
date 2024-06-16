//////////////////////////////////////////////////////////////////////
// CoordinatorSet2.h
//
//  Copyright 2/26/2024 Edgar Glowacki
//
//
//  NOTES:
//      Provides collection and management of items (set of components)
//      This used to be represented by Scene. It will iterate over
//      Coordinates, build Components row for each Coordinate and fill in 
//      all data, then iterating over specific system and pass
//      all found components.
//      There is a concept of Global coordinator (1 component type only exist)
//      in which case, we need to propagate that 'global' component
//      to rest of the rows as we iterate over.
//
//      If we are going to pass as ref from GameCoordinator to Systems,
//      then it also may cache previous frame, tick
//
//  #include "Components/CoordinatorSet2.h"
//
//////////////////////////////////////////////////////////////////////
//! \file
#pragma once


//#include "ComponentTypes.h"
#include "Components/Coordinator.h"
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

        //template <std::size_t TupleIndex, std::size_t MaxTupleSize, typename ElementType, typename... Tuple>
        //constexpr auto for_each_type()
        //{
        //    //using Tuples = std::tuple<typename Tuple::FullRow...>;

        //    //constexpr auto currentRow = std::get<TupleIndex>(Tuples{});
        //    if constexpr (TupleIndex + 1 < MaxTupleSize)
        //    {
        //        constexpr auto nextResult = for_each_type<TupleIndex + 1, MaxTupleSize, ElementType, Tuple...>();
        //        //return std::tuple_cat(currentRow, nextRow);
        //        return nextResult;
        //    }
        //    else
        //    {
        //        //return currentRow;
        //        return 0;
        //    }
        //}

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
            tuple_copy_if(source, target, [](const auto& sourceElement, const auto& targetElement)
            {
                return sourceElement != nullptr;
            });
        }

        // for_loop<std::tuple<...>>([]<std::size_t T0>()
        template<std::size_t N, typename TCallable>
        constexpr void for_loop(TCallable&& callable)
        {
            callable.template operator()<N - 1>();
            if constexpr (N - 1 > 0)
            {
                for_loop<N - 1>(std::forward<TCallable>(callable));
            }
        }

        template<typename T>
        concept requires_global_coordinator = requires { typename T::Global; };

        template <typename Coordinators, size_t Num = std::tuple_size_v<Coordinators>>
        constexpr bool has_global_coordinator()
        {
            if constexpr (Num > 0)
            {
                using CoordinatorRow = typename std::tuple_element_t<Num - 1, Coordinators>::Policy;
                CoordinatorRow coordinatorRow{};
                if constexpr (requires_global_coordinator<CoordinatorRow>)
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
            if constexpr (Num - 1 > 0)
            {
                constexpr std::size_t coordinatorIndex = Num - 1;

                using CoordinatorPolicy = typename std::tuple_element_t<coordinatorIndex, Coordinators>::Policy;

                if constexpr (internalc::requires_global_coordinator<CoordinatorPolicy>)
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

    } // namespace internalc

    template <typename... Tuple>
    struct coordinator_row_combine2
    {
        using type = decltype(internalc::coordinator_row_combine<0, std::tuple_size_v<std::remove_reference_t<std::tuple<Tuple...>>>, Tuple...>());
    };

    template<typename... Tuple>
    using coordinator_row_combine2_t = typename coordinator_row_combine2<Tuple...>::type;

    //template<char ...C>
    //requires (sizeof...(C)%2 == 0)
    //constexpr std::string
    //operator""_hex()
    template <typename... T>
    class CoordinatorSet2
    {
    public:
        using Coordinators = std::tuple<T...>;
        static constexpr size_t NumCoordinators = std::tuple_size_v<std::remove_reference_t<Coordinators>>;

        using FullRow = coordinator_row_combine2_t<T...>;
        //static_assert(meta::tuple_is_unique_v<FullRow>, "Duplicate element types in CoordinatorSet FullRow");
        const Strings mComponentNames = comp::db::GetPolicyRowNames<FullRow>();

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
                internalc::for_loop<NumCoordinators>([this, &collectedGlobalItem]<std::size_t T0>()
                {
                    constexpr std::size_t coordinatorIndex = T0;

                    using CoordinatorPolicy = typename std::tuple_element_t<coordinatorIndex, Coordinators>::Policy;

                    if constexpr (internalc::requires_global_coordinator<CoordinatorPolicy>)
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

            //---------------------------------------------------------------------------------------------
            // we need to iterate over each coordinator and collect results
            internalc::for_loop<NumCoordinators>([this, &collectedItems]<std::size_t T0>()
            {
                constexpr std::size_t coordinatorIndex = T0;

                using CoordinatorPolicy = typename std::tuple_element_t<coordinatorIndex, Coordinators>::Policy;
                if constexpr (!internalc::requires_global_coordinator<CoordinatorPolicy>)
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
                    }
                }
            });

            //---------------------------------------------------------------------------------------------
            // iterate over collected rows and trigger callback
            std::size_t numProcessedElements = 0;

            if constexpr (usesGlobal)
            {
                // only if there is no regular items (collectedItems) and we have ONE global item (collectedGlobalItem)
                if (collectedItems.empty() && !collectedGlobalItem.empty() && collectedGlobalItem.size() == 1)
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

    private:
        Coordinators mCoordinators;
    };
}
