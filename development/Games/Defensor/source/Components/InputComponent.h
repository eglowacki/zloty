
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
#include "Input/InputDevice.h"


namespace yaget::comp
{
    namespace db_input
    {
        struct Event { using Types = std::string; };

        using ValueTypes = std::tuple<Event>;

    } // namespace db_input


    class InputComponent : public db::PersistentBaseComponent<db_input::ValueTypes>
    {
    public:
        InputComponent(Id_t id, const db_input::Event::Types& event = {})
            : PersistentBaseComponent(id, std::tie(event))
        {
        }

        void AddInputEvent(const std::string& eventName, input::ActionNonParamCallback_t callback) const {}
        void AddInputEvent(const std::string& eventName, input::ActionCallback_t callback) const {}
    };

} // namespace yaget::comp
