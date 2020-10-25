///////////////////////////////////////////////////////////////////////
// InventoryComponent.h
//
//  Copyright 10/10/2020 Edgar Glowacki.
//
//  Maintained by: Edgar
//
//  NOTES:
//      Represents player inventory which owns playable pieces
//      available to player to put on the board
//
//
//  #include "InventoryComponent.h"
//
//////////////////////////////////////////////////////////////////////
//! \file
#pragma once

#include "Components/PersistentBaseComponent.h"


namespace ttt
{
    namespace ivc
    {
        struct NumPieces {};
        using Types = std::tuple<NumPieces>;
        using Storage = std::tuple<int>;
    }

    class InventoryComponent : public yaget::comp::db::PersistentBaseComponent<ivc::Types, ivc::Storage, yaget::comp::SmallPoolSize>
    {
    public:
        InventoryComponent(yaget::comp::Id_t id, int numPieces);
    };

}
