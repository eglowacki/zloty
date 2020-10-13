///////////////////////////////////////////////////////////////////////
// PlayerComponent.h
//
//  Copyright 10/10/2020 Edgar Glowacki.
//
//  Maintained by: Edgar
//
//  NOTES:
//      Represents players in current match
//
//
//  #include "PlayerComponent.h"
//
//////////////////////////////////////////////////////////////////////
//! \file
#pragma once

#include "GameTypes.h"
#include "Components/PersistentBaseComponent.h"


namespace ttt
{
    namespace pc
    {
        struct Side {};
        struct SideControl {};
        using Types = std::tuple<Side, SideControl>;
        using Storage = std::tuple<int, PieceType>;
    }

    class PlayerComponent : public yaget::comp::db::PersistentBaseComponent<pc::Types, pc::Storage, 4>
    {
    public:
        PlayerComponent(yaget::comp::Id_t id, PieceType sideControl);
    };

}
