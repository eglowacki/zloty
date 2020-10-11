///////////////////////////////////////////////////////////////////////
// PieceComponent.h
//
//  Copyright 10/10/2020 Edgar Glowacki.
//
//  Maintained by: Edgar
//
//  NOTES:
//      Individual piece representing X or O type. It starts in Inventory
//      and end up own by a board after placement
//
//
//  #include "PieceComponent.h"
//
//////////////////////////////////////////////////////////////////////
//! \file
#pragma once

#include "GameTypes.h"
#include "Components/Component.h"


namespace ttt
{
    class PieceComponent : public yaget::comp::BaseComponent
    {
    public:
        static constexpr int Capacity = 4;

        PieceComponent(yaget::comp::Id_t id, PieceType pieceType);

    private:
        PieceType mPieceType;
    };

}


namespace yaget::comp::db
{
    template <>
    struct ComponentProperties<ttt::PieceComponent>
    {
        using Row = std::tuple<>;
    };
}
