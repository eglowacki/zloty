//////////////////////////////////////////////////////////////////////
// Components/PersistentComponent.h
//
//  Copyright 7/2/2019 Edgar Glowacki
//
//  Maintained by: Edgar
//
//  NOTES:
//      Provides persistent storage (serialization) component. The variables
//      are stored in SQL (sqlite) having Component "Name" to be table name
//      and each templatize param a column name.
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
    namespace internal
    {
        //------------------------------------------------------------------------------------------------------------------------------------------------------
        // returns std::tuple<...> where param types are extracted from each element
        // which is struct with Types alias
        template <std::size_t TupleIndex, std::size_t MaxTupleSize, typename Tuple>
        constexpr auto make_persistent_storage_tuple()
        {
            using ElementType = std::tuple_element_t<TupleIndex, Tuple>;

            using ValueType = typename meta::strip_qualifiers_t<ElementType>::Types;
            std::tuple<ValueType> currentResult;

            if constexpr (TupleIndex + 1 < MaxTupleSize)
            {
                auto nextRow = std::move(make_persistent_storage_tuple<TupleIndex + 1, MaxTupleSize, Tuple>());
                return std::tuple_cat(std::move(currentResult), std::move(nextRow));

            }
            else
            {
                return currentResult;
            }
        }

        //------------------------------------------------------------------------------------------------------------------------------------------------------
        template <typename T, typename Row>
        constexpr auto& GetValue(auto& dataStorage)
        {
            // it's used to pass nice name of property, get the index
            // and then use that index into Storage tuple to get actual value.
            constexpr std::size_t index = meta::Index<T, Row>::value;
            return std::get<index>(dataStorage);
        }

    } // namespace internal

    //------------------------------------------------------------------------------------------------------------------------------------------------------
    template <typename Tuple>
    struct persistent_storage_tuple
    {
        using type = decltype(internal::make_persistent_storage_tuple<0, std::tuple_size_v<std::remove_reference_t<Tuple>>, Tuple>());
    };

    //------------------------------------------------------------------------------------------------------------------------------------------------------
    template<typename Tuple>
    using persistent_storage_tuple_t = typename persistent_storage_tuple<Tuple>::type;

    // Handles boilerplate code to manage properties and it's storage. This layout
    // gets replicated into DB schema (sqlite as a one example)
    //namespace internal
    //{
    //    struct Position { using Types = math3d::Vector3; };
    //    struct Orientation { using Types = math3d::Quaternion; };
    //    struct Scale { using Types = math3d::Vector3; };
    //
    //    using ValueTypes = std::tuple<Position, Orientation, Scale>;
    //}
    //class MyComponent : public yaget::comp::db::PersistentBaseComponent<internal::ValueTypes, yaget::comp::GlobalPoolSize>
    //
    //MyComponent(yaget::comp::Id_t id, const MyComponent::Types& params)
    //    : PersistentBaseComponent(id, params)
    //{}
    //
    //MyComponent(yaget::comp::Id_t id, const math3d::Vector3& position, const math3d::Vector3& orientation, const math3d::Vector3& scale)
    //    : PersistentBaseComponent(id, std::tie(position, orientation, scale))
    //{}
    //
    //auto position = myComponent.GetValue<internal::Position>();
    //auto orientation = myComponent.GetValue<internal::Orientation>();
    //auto scale = myComponent.GetValue<internal::Scale>();
    //

    //------------------------------------------------------------------------------------------------------------------------------------------------------
    template <typename VT, int Cap = DefaultPoolSize>
    class PersistentBaseComponent : public BaseComponent<Cap>
    {
    public:
        // Row represent a way to create name for each type
        using Row = VT;
        // Types are decltype(values) stored
        using Types = persistent_storage_tuple_t<VT>;

        template <typename T>
        constexpr const auto& GetValue() const;

        // return true if newValue is different from current one,
        // otherwise false.
        template <typename T>
        bool SetValue(const auto& newValue);

        bool SetStorage(const auto& newStorage);
        const Types& GetStorage() const { return mDataStorage; }

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


// implementation of PersistentBaseComponent
//------------------------------------------------------------------------------------------------------------------------------------------------------
template <typename VT, int Cap>
bool yaget::comp::db::PersistentBaseComponent<VT, Cap>::SetStorage(const auto& newStorage)
{
    int result = 0;

    meta::for_loop<Types>([&]<std::size_t T0>()
    {
        constexpr std::size_t index = T0;

        const auto& newValue = std::get<index>(newStorage);
        using ElementType = std::tuple_element_t<index, Row>;

        result += SetValue<ElementType>(newValue);
    });

    return result > 0;
}


//------------------------------------------------------------------------------------------------------------------------------------------------------
template <typename VT, int Cap>
template <typename T>
constexpr const auto& yaget::comp::db::PersistentBaseComponent<VT, Cap>::GetValue() const
{
    return internal::GetValue<T, Row>(mDataStorage);
}


//------------------------------------------------------------------------------------------------------------------------------------------------------
template <typename VT, int Cap>
template <typename T>
bool yaget::comp::db::PersistentBaseComponent<VT, Cap>::SetValue(const auto& newValue)
{
    bool result = false;

    auto& currentValue = internal::GetValue<T, Row>(mDataStorage);
    if ((result = (currentValue != newValue)))
    {
        const auto oldValue = currentValue;
        currentValue = newValue;
    }

    return result;
}
