//////////////////////////////////////////////////////////////////////
// Components/PersistentComponent.h
//
//  Copyright 7/2/2019 Edgar Glowacki
//
//  Maintained by: Edgar
//
//  NOTES:
//      Class that handles atomic smart pointer exchange
//      between threads
//
//
//  #include "Components/PersistentComponent.h"
//
//////////////////////////////////////////////////////////////////////
//! \file
#pragma once

#include "Components/Component.h"


namespace yaget::comp::db
{
    //template <typename T>
    //using get_t = decltype(std::declval<T>().Foo());

    //template <typename >
    //using supports_get = std::is_detected<get_t, T>;

    // Handles boiler plate code to manage properties and it's storage
    template <typename PT, typename PS, int Cap = 64>
    class PersistentBaseComponent : public BaseComponent<Cap>
    {
    public:
        using Row = PT;
        using Types = PS;

        template <typename... T>
        PersistentBaseComponent(Id_t id, T... args)
            : BaseComponent<Cap>(id)
            , mDataStorage(args...)
        {}

        template <typename T>
        const auto& GetValue() const
        {
            // it's used to pass nice name of property, get the index
            // and then ise that index into Storage tuple to get actual value.
            constexpr std::size_t index = meta::Index<T, Row>::value;
            return std::get<index>(mDataStorage);
        }

    private:
        Types mDataStorage;
    };
}
