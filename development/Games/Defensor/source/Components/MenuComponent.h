
/////////////////////////////////////////////////////////////////////////
// MenuComponent.h
//
//  Copyright 8/04/2024 Edgar Glowacki.
//
// NOTES:
//      
//
//
// #include "Components/MenuComponent.h"
//
/////////////////////////////////////////////////////////////////////////
//! \file
#pragma once

#include "Components/PersistentBaseComponent.h"

namespace yaget::comp
{
    namespace db_menu
    {
        struct Event { using Types = std::string; };

        using ValueTypes = std::tuple<Event>;

    } // namespace db_menu


    class MenuComponent : public db::PersistentBaseComponent<db_menu::ValueTypes>
    {
    public:
        MenuComponent(Id_t id)
            : PersistentBaseComponent(id)
        {
        }

        void OnInput(const std::string& eventName);

        using InputQueue = std::map<std::string, bool>;
        InputQueue mInputQueue;
    };

} // namespace yaget::comp
