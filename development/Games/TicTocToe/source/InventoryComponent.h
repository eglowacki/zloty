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

#include "Components/Component.h"


namespace ttt
{
    class InventoryComponent : public yaget::comp::BaseComponent
    {
    public:
        static constexpr int Capacity = 4;

        InventoryComponent(yaget::comp::Id_t id, int numPieces);

    private:
        int mNumPieces;
    };

}

namespace yaget::comp::db
{
    template <>
    struct ComponentProperties<ttt::InventoryComponent>
    {
        using Row = std::tuple<>;
    };
}



