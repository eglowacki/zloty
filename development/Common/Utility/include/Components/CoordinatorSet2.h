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
        template<typename T>
        concept is_global = requires { typename T::Policy::Global; };

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
            using CurrentResult = std::tuple_element_t<TupleIndex, TupleA>;

            if constexpr (TupleIndex + 1 < MaxTupleSize && meta::tuple_is_element_v<CurrentResult, TupleB>)
            {
                constexpr auto nextResult = tuple_get_union<TupleA, TupleB, TupleIndex + 1, MaxTupleSize>();
                return std::tuple_cat(std::tuple<CurrentResult>{}, nextResult);
            }
            else
            {
                if constexpr (meta::tuple_is_element_v<CurrentResult, TupleB>)
                {
                    return std::tuple<CurrentResult>{};
                }
                else
                {
                    return std::tuple<>{};
                }
            }
        }

        // Copy tuple from Source to Target overwriting any values in Target
        template<typename S, typename T, int N = std::tuple_size_v<std::remove_reference_t<S>>>
        constexpr void tuple_copy(const S& source, T& target)
        {
            using ET = std::tuple_element_t<N - 1, S>;

            std::get<ET>(target) = std::get<ET>(source);

            if constexpr (N - 1 > 0)
            {
                tuple_copy<S, T, N - 1>(source, target);
            }
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


    } // namespace internalc

    template <typename... Tuple>
    struct coordinator_row_combine2
    {
        using type = decltype(internalc::coordinator_row_combine<0, std::tuple_size_v<std::remove_reference_t<std::tuple<Tuple...>>>, Tuple...>());
    };

    template<typename... Tuple>
    using coordinator_row_combine2_t = typename coordinator_row_combine2<Tuple...>::type;

    template<typename TupleA, typename TupleB>
    using tuple_get_union_t = decltype(internalc::tuple_get_union<TupleA, TupleB, 0, std::tuple_size_v<TupleA>>());

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
        template <typename QueryRow>
        using RowCallback = std::function<bool(Id_t, QueryRow)>;

        template <typename QueryRow>
        std::size_t ForEach(RowCallback<QueryRow> callback)
        {
            // we want to compare QueryRow against each Coordinator::FullRow,
            // and collect same elements into one tuple type and return that.
            // with this type we can enumerate over all rows using returned tuple.

            using Items = std::map<comp::Id_t, QueryRow>;
            Items collectedItems;

            std::size_t numItemsResult = 0;

            // we need to iterate over each coordinator and collect results
            //---------------------------------------------------------------------------------------------

            internalc::for_loop<NumCoordinators>([this, &collectedItems]<std::size_t T0>()
            {
                constexpr std::size_t coordinatorIndex = T0;

                using CoordinatorRow = typename std::tuple_element_t<coordinatorIndex, Coordinators>::FullRow;
                using RequestedRow = tuple_get_union_t<QueryRow, CoordinatorRow>;

                QueryRow queryRow{};
                CoordinatorRow coordinatorRow{};
                RequestedRow requestedRow{};

                struct RequestedRowPolicy { using Row = RequestedRow; };

                if constexpr (std::tuple_size_v<RequestedRow> > 0)
                {
                    auto& coordinator = GetCoordinator<coordinatorIndex>();
                    const std::size_t numItems = coordinator.template ForEach<RequestedRowPolicy>([&collectedItems](comp::Id_t id, const auto& row)
                    {
                        internalc::tuple_copy(row, collectedItems[id]);
                        return true;
                    });
                }
            });

            return numItemsResult;
        }

        //template <typename C>                 ^
        //comp::Coordinator<C>& GetCoordinator()
        //{
        //    return std::get<comp::Coordinator<C>>(mCoordinators);
        //}

        //template <typename C>
        //const comp::Coordinator<C>& GetCoordinator() const
        //{
        //    return std::get<comp::Coordinator<C>>(mCoordinators);
        //}

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

        //template <typename TCoordinator>
        //constexpr std::size_t GetCoordinatorIndex() const
        //{
        //    return meta::Index<TCoordinator, Coordinators>::value;
        //}

    private:
        Coordinators mCoordinators;
    };
}
