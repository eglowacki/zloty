﻿//////////////////////////////////////////////////////////////////////
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


#include "ComponentTypes.h"
#include "Meta/Hana.h"
#include <boost/hana/ext/std/tuple.hpp>
#include <functional>


YAGET_COMPILE_WARNING_LEVEL_START(3, "Development of CoordinatorSet class")

namespace yaget::comp
{
    namespace internal
    {
        template<typename T>
        concept is_global = requires { typename T::Policy::Global; };
    }


    template <typename... T>
    class CoordinatorSet
    {
    public:
        template <typename R>
        std::size_t ForEach(std::function<bool(Id_t id, R components)> callback)
        {
            namespace hana = boost::hana;
            using RequestRow = R;

            if constexpr (NumCoordinators == 1)
            {
                // nice optimization in case only 1 coordinator
                return std::get<0>(mCoordinators).template ForEach<RequestRow>(callback);
            }

            RequestRow templateRow{};
            using Rows = std::map<Id_t, RequestRow>;
            Rows rows;

            // walk over each coordinator and extract which RequestRow component belong to coordinator
            // and construct RowPolicy to be ingested by coordinator template args
            meta::for_each(mCoordinators, [&rows, &templateRow]<typename C>(C& coordinator)
            {
                using CoordType = C;
                using CoordinatorRow = decltype(hana::to_tuple(typename CoordType::Row{}));

                // filter user request row by which coordinator posses that component. There is a guaranty of no duplicate types between coordinators
                constexpr auto transformedRow = hana::filter(hana::to_tuple(RequestRow{}), []<typename RR>(const RR&)
                {
                    constexpr auto hasComponent = hana::contains(CoordinatorRow{}, RR{}) == 1;
                    if constexpr (hasComponent)
                    {
                        return hana::bool_c<true>;
                    }
                    else
                    {
                        return hana::bool_c<false>;
                    }
                });

                // simple optimization of not doing anything for this coordinator
                // it no RowPolicy
                if constexpr (hana::size(transformedRow))
                {
                    // create our RowPolicy object and get type
                    constexpr auto qrow = boost::hana::unpack(transformedRow, [](auto... args)
                    {
                        using RP = comp::RowPolicy<decltype(args)...>;
                        return RP{};
                    });
                    using QueryRow = decltype(qrow);

                    std::size_t numItems = coordinator.template ForEach<QueryRow>([&rows, &templateRow](comp::Id_t id, const auto& row)
                    {
                        if constexpr (internal::is_global<CoordType>)
                        {
                            meta::tuple_copy(row, templateRow);
                        }
                        else
                        {
                            // get ref to the row we need to modify and copy from incoming row in to it
                            // we don't worry about stomping already set fields, since ForEach call is per item (unique id)
                            // and only the one for this specific coordinator (there is no overlap between coordinators per types)
                            RequestRow& currentRow = rows[id];
                            meta::tuple_copy(row, currentRow);
                        }

                        return true;
                    });
                }
            });

            // Now,call call back for each element in rows
            int itemCounter = 0;
            for (const auto& [id, row] : rows)
            {
                ++itemCounter;

                RequestRow requestRow = row;
                meta::tuple_copy(templateRow, requestRow);

                if (!callback(id, requestRow))
                {
                    return itemCounter;
                }
            }

            return itemCounter;
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

        //// Iterate over each item in ids collection and call callback on each item
        //template<typename R>
        //void ForEach(const comp::ItemIds& ids, std::function<bool(comp::Id_t id, const typename R::Row& row)> callback)
        //{
        //    
        //}

        //// Iterate over all items that conform to pattern R
        //// Return number of matched items, or 0 if none.
        //template<typename... R>
        //std::size_t ForEach(std::function<bool(comp::Id_t id, const typename Row& row)> callback)
        //{
        //    
        //}

    private:
        using Coordinators = std::tuple<T...>;
        static constexpr size_t NumCoordinators = std::tuple_size_v<std::remove_reference_t<Coordinators>>;

        Coordinators mCoordinators;
    };
}

YAGET_COMPILE_WARNING_LEVEL_END