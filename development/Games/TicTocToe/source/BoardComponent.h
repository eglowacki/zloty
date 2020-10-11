///////////////////////////////////////////////////////////////////////
// BoardComponent.h
//
//  Copyright 10/10/2020 Edgar Glowacki.
//
//  Maintained by: Edgar
//
//  NOTES:
//      Manages board state. This usually will be one object of this type
//
//
//  #include "BoardComponent.h"
//
//////////////////////////////////////////////////////////////////////
//! \file
#pragma once

#include "Components/Component.h"


namespace ttt
{
    class BoardComponent : public yaget::comp::BaseComponent
    {
    public:
        static constexpr int Capacity = 4;

        BoardComponent(yaget::comp::Id_t id, int rows, int columns);

    private:
        int mNumRows;
        int mNumColumns;
    };

}

namespace yaget::comp::db
{
    template <>
    struct ComponentProperties<ttt::BoardComponent>
    {
        using Row = std::tuple<>;
    };
}


