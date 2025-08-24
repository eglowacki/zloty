
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
//#include "Input/InputDevice.h"


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
        InputComponent(Id_t id, const db_input::ActionNames::Types& event = {})
            : PersistentBaseComponent(id, std::tie(event))
        {}

        std::set<std::string> mTriggeredAction;
    };

} // namespace yaget::comp
