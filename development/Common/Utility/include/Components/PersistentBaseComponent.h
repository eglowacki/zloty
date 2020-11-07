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

#include <utility>


#include "Components/Component.h"


namespace yaget::comp::db
{
    //template <typename T>
    //using get_t = decltype(std::declval<T>().Foo());

    //template <typename >
    //using supports_get = std::is_detected<get_t, T>;

    // Handles boiler plate code to manage properties and it's storage
    //namespace pc
    //{
    //    struct Side {};
    //    struct SideControl {};
    //    using Types = std::tuple<Side, SideControl>;
    //    using Storage = std::tuple<int, PieceType>;
    //}
    //class MyComponent : public yaget::comp::db::PersistentBaseComponent<sc::Types, sc::Storage, yaget::comp::GlobalPoolSize>
    //
    //MyComponent(yaget::comp::Id_t id, const pc::Storage& params)
    //    : PersistentBaseComponent(id, params)
    //{}
    //
    //MyComponent(yaget::comp::Id_t id, int param1, PieceType param2)
    //    : PersistentBaseComponent(id, std::tie(param1, param2))
    //{}
    //
    template <typename PT, typename PS, int Cap = 64>
    class PersistentBaseComponent : public BaseComponent<Cap>
    {
    public:
        // Row represent a way to create name for each type
        using Row = PT;
        // Types are decltype(values) stored
        using Types = PS;

        template <typename T>
        constexpr const auto& GetValue() const;

    protected:
        PersistentBaseComponent(Id_t id)
            : PersistentBaseComponent(id, Types{})
        {}

        PersistentBaseComponent(Id_t id, Types params)
            : BaseComponent<Cap>(id)
            , mDataStorage(std::move(params))
        {}

    private:
        Types mDataStorage;
    };
}


//  #include "Components/PersistentComponentImplementation.h"
namespace yaget::comp::db
{

    template <typename PT, typename PS, int Cap>
    template <typename T>
    constexpr const auto& PersistentBaseComponent<PT, PS, Cap>::GetValue() const
    {
        // it's used to pass nice name of property, get the index
        // and then use that index into Storage tuple to get actual value.
        constexpr std::size_t index = meta::Index<T, Row>::value;
        return std::get<index>(mDataStorage);
    }
}
