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

        static const Id_t INVALID_ID = 0;
        static const Id_t END_ID_MARKER = std::numeric_limits<Id_t>::max() - 1;

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




