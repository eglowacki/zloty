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

        // Any id's that are marked as persistent, will have high bit set
        static constexpr const Id_t PERSISTENT_ID_BIT = 0x8000000000000000;

        static constexpr const Id_t INVALID_ID = 0;
        static constexpr const Id_t END_ID_MARKER = (~PERSISTENT_ID_BIT) - 1;
        static constexpr const Id_t GLOBAL_ID_MARKER = ~PERSISTENT_ID_BIT;

        constexpr Id_t StripQualifiers(Id_t id)
        {
            return ~PERSISTENT_ID_BIT & id;
        }

        constexpr bool IsIdValid(Id_t id)
        {
            const auto& sid = StripQualifiers(id);
            return !(sid == INVALID_ID || sid == END_ID_MARKER || sid == GLOBAL_ID_MARKER || id == PERSISTENT_ID_BIT);
        }

        constexpr bool IsIdPersistent(Id_t id)
        {
            return IsIdValid(id) && (id & PERSISTENT_ID_BIT) == PERSISTENT_ID_BIT;
        }

        inline Id_t MarkAsPersistent(Id_t id)
        {
            YAGET_ASSERT(IsIdValid(id), "id: '%d' is invalid and can not be marked as persistent.", id);
            return PERSISTENT_ID_BIT | id;
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
            static constexpr size_t NumComponents = std::tuple_size_v<std::remove_reference_t<Row>>;

            static_assert(NumComponents > 0, "Policy must have at least one Element.");
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





