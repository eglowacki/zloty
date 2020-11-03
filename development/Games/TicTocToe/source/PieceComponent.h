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

#include "Components/PersistentBaseComponent.h"


namespace ttt
{
    enum class PieceType;

    namespace pic
    {
        struct Side {};
        using Types = std::tuple<Side>;
        using Storage = std::tuple<int>;
    }

    class PieceComponent : public yaget::comp::db::PersistentBaseComponent<pic::Types, pic::Storage>
    {
    public:
        PieceComponent(yaget::comp::Id_t id, const pic::Storage& pieceType)
        : PersistentBaseComponent(id, pieceType)
        {}
    };

}
