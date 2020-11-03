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

#include "Components/PersistentBaseComponent.h"


namespace ttt
{
    namespace bc
    {
        struct Rows {};
        struct Columns {};
        using Types = std::tuple<Rows, Columns>;
        using Storage = std::tuple<int, int>;
    }

    class BoardComponent : public yaget::comp::db::PersistentBaseComponent<bc::Types, bc::Storage, yaget::comp::GlobalPoolSize>
    {
    public:
        BoardComponent(yaget::comp::Id_t id, const bc::Storage& params)
        : PersistentBaseComponent(id, params)
        {}
    };

}
