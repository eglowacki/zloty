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

#include "Components/PersistentBaseComponent.h"


namespace ttt
{
    namespace sc
    {
        using Types = std::tuple<>;
        using Storage = std::tuple<>;
    }

    class ScoreComponent : public yaget::comp::db::PersistentBaseComponent<sc::Types, sc::Storage, yaget::comp::GlobalPoolSize>
    {
    public:
        ScoreComponent(yaget::comp::Id_t id);
    };

}
