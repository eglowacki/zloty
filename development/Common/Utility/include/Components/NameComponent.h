//////////////////////////////////////////////////////////////////////
// NameComponent.h
//
//  Copyright 7/18/2025 Edgar Glowacki
//
//  Maintained by: Edgar
//
//  NOTES:
//      Provides naming of items/entities. The name can be changed
//      at any time, since all internal systems refer to
//      items/entities by id.
//
//  #include "Components/NameComponent.h"
//
//////////////////////////////////////////////////////////////////////
//! \file
#pragma once

#include "Components/PersistentObserverComponent.h"


namespace yaget::comp
{

    namespace db_name
    {
        struct Name { using Types = std::string; };

        using ValueTypes = std::tuple<Name>;
    }

    class NameComponent : public db::PersistentObserverComponent<db_name::ValueTypes>
    {
    public:
        NameComponent(Id_t id, const db_name::Name::Types& name)
            : PersistentObserverComponent(id, std::tie(name))
        {}
    };

} // namespace yaget::comp
