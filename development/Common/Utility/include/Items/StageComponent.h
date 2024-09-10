///////////////////////////////////////////////////////////////////////
// StageComponent.h
//
//  Copyright 9/7/2024 Edgar Glowacki.
//
//  Maintained by: Edgar
//
//  NOTES:
//      Uses 'Stage' table from Director to manage and control
//		loading and unloading items/components.
//
//
//  #include "Items/StageComponent.h"
//
//////////////////////////////////////////////////////////////////////
//! \file
#pragma once

#include "Components/PersistentBaseComponent.h"

namespace yaget::items
{
    namespace db_stage
    {
        struct StageName { using Types = std::string; };

        using ValueTypes = std::tuple<StageName>;

    } // namespace db_stage


    class StageComponent : public comp::db::PersistentBaseComponent<db_stage::ValueTypes>
    {
    public:
        StageComponent(comp::Id_t id, const db_stage::StageName::Types& stageName)
            : PersistentBaseComponent(id, std::tie(stageName))
        {}

    };

} // namespace yaget::items


