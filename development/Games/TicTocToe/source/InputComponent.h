///////////////////////////////////////////////////////////////////////
// InputComponent.h
//
//  Copyright 10/10/2020 Edgar Glowacki.
//
//  Maintained by: Edgar
//
//  NOTES:
//      Represents player input
//
//
//  #include "InputComponent.h"
//
//////////////////////////////////////////////////////////////////////
//! \file
#pragma once

#include "Components/PersistentBaseComponent.h"


namespace ttt
{
    namespace ic
    {
        using Types = std::tuple<>;
        using Storage = std::tuple<>;
    }

    class InputComponent : public yaget::comp::db::PersistentBaseComponent<ic::Types, ic::Storage, yaget::comp::SmallPoolSize>
    {
    public:
        InputComponent(yaget::comp::Id_t id)
            : PersistentBaseComponent(id)
        {}
    };

}
