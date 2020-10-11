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
#include "Components/Component.h"


namespace ttt
{
    class PlayerComponent : public yaget::comp::BaseComponent
    {
    public:
        static constexpr int Capacity = 4;

        PlayerComponent(yaget::comp::Id_t id, PieceType sideControl);

    private:
        PieceType mSideControl;
    };

}

namespace yaget::comp::db
{
    struct Side {};

    template <>
    struct ComponentProperties<ttt::PlayerComponent>
    {
        using Row = std::tuple<Side>;
        using Types = std::tuple<int>;
        static Types DefaultRow() { return Types{ 1 }; }
    };
}
