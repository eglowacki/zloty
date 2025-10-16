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

#include "Components/PersistentObserverComponent.h"
#include "Core/ErrorHandlers.h"


namespace yaget::items
{
    namespace db_stage
    {
        struct Name { using Types = std::string; };
        // it would be nice to have a way to actually use enum, and having serializer handle that
        // 0 - remove all from previous stage and add this.
        // 1 - Remove items from previous stage that are not in current stage and then add/update
        // 2 - keep all and just add/update
        enum class BlendOp { Replace, MergeSame, MergeAll };
        struct Blend { using Types = BlendOp; };

        using ValueTypes = std::tuple<Name, Blend>;

    } // namespace db_stage


    class StageComponent : public comp::db::PersistentObserverComponent<db_stage::ValueTypes, comp::GlobalPoolSize>
    {
    public:
        StageComponent(comp::Id_t id, const db_stage::Name::Types& name, const db_stage::Blend::Types& blend = db_stage::Blend::Types::Replace)
            : PersistentObserverComponent(id, std::tie(name, blend))
        {}
    };

} // namespace yaget::items

namespace yaget::conv
{
    //----------------------------------------------------------------------------------------------------------------------------------
    template <>
    struct Convertor<items::db_stage::BlendOp>
    {
        static items::db_stage::BlendOp FromString(const char* value)
        {
            items::db_stage::BlendOp result = items::db_stage::BlendOp::Replace;

            if (value)
            {
                if (std::strcmp(value, "Replace") == 0)
                {
                    result = items::db_stage::BlendOp::Replace;
                }
                else if (strcmp(value, "MergeSame") == 0)
                {
                    result = items::db_stage::BlendOp::MergeSame;
                }
                else if (strcmp(value, "MergeAll") == 0)
                {
                    result = items::db_stage::BlendOp::MergeAll;
                }
                else
                {
                    error_handlers::Throw("enum BlendOp is out of range");
                }
            }
            return result;
        }

        static std::string ToString(items::db_stage::BlendOp value)
        {
            std::string result;

            switch (value)
            {
            case items::db_stage::BlendOp::Replace:
                result = "Replace";
                break;
            case items::db_stage::BlendOp::MergeSame:
                result = "MergeSame";
                break;
            case items::db_stage::BlendOp::MergeAll:
                result = "MergeAll";
                break;
            default:
                error_handlers::Throw("enum BlendOp is out of range");

            }

            return result;
        }
    };
    
}

