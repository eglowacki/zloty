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
        struct Items { using Types = Strings; };

        using ValueTypes = std::tuple<Items>;
    }


    //------------------------------------------------------------------------------------------------------------------------------------------------------
    class MenuScreenComponent : public db::PersistentObserverComponent<db_menu_screen::ValueTypes>
    {
    public:
        MenuScreenComponent(Id_t id, const db_menu_screen::Items::Types& items = {})
            : PersistentObserverComponent(id, std::tie(items))
        {}
    };

} // namespace yaget::comp
