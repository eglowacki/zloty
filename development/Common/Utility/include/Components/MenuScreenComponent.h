//////////////////////////////////////////////////////////////////////
// MenuScreenComponent.h
//
//  Copyright 5/19/2026 Edgar Glowacki
//
//  Maintained by: Edgar
//
//  NOTES:
//
//  #include "Components/MenuScreenComponent.h"
//
//////////////////////////////////////////////////////////////////////
//! \file
#pragma once

#include "Components/PersistentObserverComponent.h"


namespace yaget::comp
{
    //------------------------------------------------------------------------------------------------------------------------------------------------------
    namespace db_menu_screen
    {
        struct Name { using Types = std::string; };

        using ValueTypes = std::tuple<Name>;
    }


    //------------------------------------------------------------------------------------------------------------------------------------------------------
    class MenuScreenComponent : public db::PersistentObserverComponent<db_menu_screen::ValueTypes>
    {
    public:
        MenuScreenComponent(Id_t id, const db_menu_screen::Name::Types& name)
            : PersistentObserverComponent(id, std::tie(name))
        {}
    };

} // namespace yaget::comp
