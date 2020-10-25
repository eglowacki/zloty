//////////////////////////////////////////////////////////////////////
// ComponentTypes.h
//
//  Copyright 7/1/2019 Edgar Glowacki
//
//  Maintained by: Edgar
//
//  NOTES:
//      
//
//
//  #include "Components/ComponentTypes.h"
//
//////////////////////////////////////////////////////////////////////
//! \file
#pragma once

#include "YagetCore.h"
#include <set>


namespace yaget
{
    namespace comp
    {
        // represents unique id of entity (item) and every components
        // which entity is composed of
        using Id_t = uint64_t;

        using ItemIds = std::set<comp::Id_t>;

        static constexpr const Id_t INVALID_ID = 0;
        static constexpr const Id_t END_ID_MARKER = std::numeric_limits<Id_t>::max() - 1;
        static constexpr const Id_t GLOBAL_ID_MARKER = std::numeric_limits<Id_t>::max();

        inline bool IsIdValid(Id_t id)
        {
            return !(id == INVALID_ID || id == END_ID_MARKER);
        }

        // provides layout and types of entity components (Item)
        // IS... var args represent list of classes (aka components) that one item represents at it's fullest
        // Not all items will have all components fill in
        // Ex: RowPolicy<Location, Physics, Script>
        //      most likely all components will have location, majority will have Physics and few if any may contain Script
        //
        // You can use this type of pattern to decorate RowPolicy
        //struct GlobalEntity : yaget::comp::RowPolicy<BoardComponent*, ScoreComponent*>
        //{
        //    using AutoCleanup = bool;
        //    using Global = bool;
        //};
        //
        template <typename... IS>
        struct RowPolicy
        {
            using Row = std::tuple<IS...>;
            static_assert(std::tuple_size_v<std::remove_reference_t<Row>> > 0, "Policy must have at least one Element.");
            static_assert(meta::tuple_is_unique_v<Row>, "Duplicate element types in Policy");
        };

        // game system
        namespace gs
        {
            // Used a as policy in GameSystem to handle last entity/item during iteration
            // I - index slot used in GameCoordinator to use with this system. Current convention
            // is 0 Global, 1  is Entities
            // T how the last and end element iteration is handled
            template <int I, bool T>
            struct EndMarker
            {
                static constexpr bool val = true;
                static constexpr int id = I;
            };

            template <int I>
            struct EndMarker<I, false>
            {
                static constexpr bool val = false;
                static constexpr int id = I;
            };

            // Used to specify how to handle end of items in GameSystem::Update method. 
            // If Yes, then call Update with comp::END_ID_MARKER, otherwise
            // skip that after iterating over all ids.
            template <int I>
            struct EndMarkerYes : EndMarker<I, true> {};
            template <int I>
            struct EndMarkerNo : EndMarker<I, false> {};

        } // namespace gs

    } // namespace comp
} // namespace yaget





