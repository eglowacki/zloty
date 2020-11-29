///////////////////////////////////////////////////////////////////////
// AssetComponent.h
//
//  Copyright 11/24/2020 Edgar Glowacki.
//
//  Maintained by: Edgar
//
//  NOTES:
//      Manages which assets are used for rendering,
//      used by logic (game) thread
//
//
//  #include "AssetComponent.h"
//
//////////////////////////////////////////////////////////////////////
//! \file
#pragma once

#include "Components/PersistentBaseComponent.h"

#include "VTS/VirtualTransportSystem.h"


namespace ttt
{
    namespace ac
    {
        struct Section {};
        using Types = std::tuple<Section>;
        using Storage = std::tuple<yaget::io::VirtualTransportSystem::Section>;
    }

    class AssetComponent : public yaget::comp::db::PersistentBaseComponent<ac::Types, ac::Storage>
    {
    public:
        using Section = yaget::io::VirtualTransportSystem::Section;

        AssetComponent(yaget::comp::Id_t id, const Section& section)
        : PersistentBaseComponent(id, std::tie(section))
        {}
    };

}
