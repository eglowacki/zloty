///////////////////////////////////////////////////////////////////////
// ScoreComponent.h
//
//  Copyright 10/10/2020 Edgar Glowacki.
//
//  Maintained by: Edgar
//
//  NOTES:
//      Manages standing, score of match
//
//
//  #include "ScoreComponent.h"
//
//////////////////////////////////////////////////////////////////////
//! \file
#pragma once

#include "Components/Component.h"


namespace ttt
{
    class ScoreComponent : public yaget::comp::BaseComponent
    {
    public:
        static constexpr int Capacity = 4;

        ScoreComponent(yaget::comp::Id_t id);
    };

}


namespace yaget::comp::db
{
    template <>
    struct ComponentProperties<ttt::ScoreComponent>
    {
        using Row = std::tuple<>;
    };
}


