
/////////////////////////////////////////////////////////////////////////
// MaterialComponent.h
//
//  Copyright 2/7/2026 Edgar Glowacki.
//
// NOTES:
//      
//
//
// #include "Components/MaterialComponent.h"
//
/////////////////////////////////////////////////////////////////////////
//! \file
#pragma once

#include "Components/PersistentBaseComponent.h"
#include "VTS/ResolvedAssets.h"

namespace yaget::comp
{
    namespace db_material
    {
        struct Section { using Types = io::VirtualTransportSystem::Section; };

        using ValueTypes = std::tuple<Section>;

    }


    class MaterialComponent : public db::PersistentBaseComponent<db_material::ValueTypes>
    {
    public:
        MaterialComponent(Id_t id, const db_material::Section::Types& section);

        io::Tag mAssetTag;

    private:
    };

} // namespace yaget::comp
