
/////////////////////////////////////////////////////////////////////////
// InputComponent.h
//
//  Copyright 8/02/2024 Edgar Glowacki.
//
// NOTES:
//      
//
//
// #include "Components/InputComponent.h"
//
/////////////////////////////////////////////////////////////////////////
//! \file
#pragma once

#include "Components/PersistentBaseComponent.h"


namespace yaget::comp
{
    namespace db_input
    {
        struct ActionNames { using Types = Strings; };

        using ValueTypes = std::tuple<ActionNames>;

    } // namespace db_input


    class InputComponent : public db::PersistentBaseComponent<db_input::ValueTypes>
    {
    public:
        struct ActionInput
        {
            std::string mName;
            uint64_t mTimeStamp;
            int32_t mMouseX;
            int32_t mMouseY;
            uint32_t mFlags;
        };

        InputComponent(Id_t id, const db_input::ActionNames::Types& event = {})
            : PersistentBaseComponent(id, std::tie(event))
        {}

        bool IsAction(const std::string& actionName) const
        {
            return mTriggeredAction.contains(actionName);
        }

        std::map<std::string, ActionInput> mTriggeredAction;
        //std::set<std::string> mTriggeredAction;
    };

} // namespace yaget::comp
