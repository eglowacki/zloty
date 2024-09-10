
/////////////////////////////////////////////////////////////////////////
// UnitComponent.h
//
//  Copyright 8/02/2024 Edgar Glowacki.
//
// NOTES:
//      
//
//
// #include "Components/UnitComponent.h"
//
/////////////////////////////////////////////////////////////////////////
//! \file
#pragma once

#include "Components/PersistentBaseComponent.h"

namespace yaget::comp
{
    namespace db_unit
    {
        struct ShipType { using Types = int; };

        using ValueTypes = std::tuple<ShipType>;

    } // namespace db_unit


    class UnitComponent : public db::PersistentBaseComponent<db_unit::ValueTypes>
    {
    public:
        UnitComponent(Id_t id, const db_unit::ShipType::Types& shipType)
            : PersistentBaseComponent(id, std::tie(shipType))
        {
        }
        
    };

} // namespace yaget::comp
