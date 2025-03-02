﻿///////////////////////////////////////////////////////////////////////
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
        struct Name {};
        using Types = std::tuple<Side, Name>;
        using Storage = std::tuple<int, std::string>;
    }

    class PlayerComponent : public yaget::comp::db::PersistentBaseComponent<pc::Types, pc::Storage, yaget::comp::SmallPoolSize>
    {                                                            
    public:
        PlayerComponent(yaget::comp::Id_t id, int side, const std::string& name)
            : PersistentBaseComponent(id, std::tie(side, name))
        {}
    };

}
