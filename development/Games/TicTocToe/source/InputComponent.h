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

#include "ScoreComponent.h"

#include "Components/Component.h"


namespace ttt
{
    class InputComponent : public yaget::comp::BaseComponent
    {
    public:
        static constexpr int Capacity = 4;

        InputComponent(yaget::comp::Id_t id);
    };

}


namespace yaget::comp::db
{
    template <>
    struct ComponentProperties<ttt::InputComponent>
    {
        using Row = std::tuple<>;
    };
}

