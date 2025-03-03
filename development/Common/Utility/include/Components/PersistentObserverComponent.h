//////////////////////////////////////////////////////////////////////
// Components/PersistentObserverComponent.h
//
//  Copyright 7/2/2019 Edgar Glowacki
//
//  Maintained by: Edgar
//
//  NOTES:
//      Provides persistent storage (serialization) component, including
//      callbacks for data storage changes
//
//  #include "Components/PersistentObserverComponent.h"
//
//////////////////////////////////////////////////////////////////////
//! \file
#pragma once

#include <utility>


#include "Components/PersistentBaseComponent.h"


namespace yaget::comp::db
{
    //------------------------------------------------------------------------------------------------------------------------------------------------------
    using ObserverHandle = void*;
    template <typename T>
    using ObserverFunction = std::function<void(const typename T::Types& oldValue, const typename T::Types& newValue)>;

    namespace internal
    {
        //------------------------------------------------------------------------------------------------------------------------------------------------------
        // Return tuple of observers callback signatures
        template <std::size_t TupleIndex, std::size_t MaxTupleSize, typename Tuple>
        constexpr auto make_observers_tuple()
        {
            using ParamType = typename std::tuple_element_t<TupleIndex, Tuple>;

            std::tuple<comp::db::ObserverFunction<ParamType>> observer;

            if constexpr (TupleIndex + 1 < MaxTupleSize)
            {
                auto nextObserver = make_observers_tuple<TupleIndex + 1, MaxTupleSize, Tuple>();
                return std::tuple_cat(observer, nextObserver);

            }
            else
            {
                return observer;
            }
        }
    }

    //------------------------------------------------------------------------------------------------------------------------------------------------------
    template <typename Tuple>
    struct make_observers_tuple
    {
        using type = decltype(internal::make_observers_tuple<0, std::tuple_size_v<std::remove_reference_t<Tuple>>, Tuple>());
    };

    //------------------------------------------------------------------------------------------------------------------------------------------------------
    template<typename Tuple>
    using make_observers_tuple_t = typename make_observers_tuple<Tuple>::type;

    //------------------------------------------------------------------------------------------------------------------------------------------------------
    template <typename VT, int Cap = DefaultPoolSize>
    class PersistentObserverComponent : public PersistentBaseComponent<VT, Cap>
    {
    public:
        using Row = typename PersistentBaseComponent<VT, Cap>::Row;
        using Types = typename PersistentBaseComponent<VT, Cap>::Types;

        // return true if newValue is different from current one,
        // otherwise false.
        template <typename T>
        bool SetValue(const auto& newValue);

        bool SetStorage(const auto& newStorage);

        template <typename T>
        ObserverHandle Connect(ObserverFunction<T> observerFunction);

    protected:
        PersistentObserverComponent(Id_t id)
            : PersistentBaseComponent<VT, Cap>(id)
        {}

        PersistentObserverComponent(Id_t id, Types params)
            : PersistentBaseComponent<VT, Cap>(id, params)
        {}

    private:
        using Observers = make_observers_tuple_t<Row>;
        Observers mObservers;
    };

}


// implementation of PersistentObserverComponent
//------------------------------------------------------------------------------------------------------------------------------------------------------
template <typename VT, int Cap>
template <typename T>
yaget::comp::db::ObserverHandle yaget::comp::db::PersistentObserverComponent<VT, Cap>::Connect(ObserverFunction<T> observerFunction)
{
    std::get<ObserverFunction<T>>(mObservers) = std::move(observerFunction);
    return &std::get<ObserverFunction<T>>(mObservers);
}


//------------------------------------------------------------------------------------------------------------------------------------------------------
template <typename VT, int Cap>
template <typename T>
bool yaget::comp::db::PersistentObserverComponent<VT, Cap>::SetValue(const auto& newValue)
{
    bool result = false;

    const auto oldValue = PersistentBaseComponent<VT, Cap>::template GetValue<T>();

    if ((result = PersistentBaseComponent<VT, Cap>::template SetValue<T>(newValue)))
    {
        if (auto& observer = std::get<ObserverFunction<T>>(mObservers))
        {
            observer(oldValue, newValue);
        }
    }

    return result;
}


//------------------------------------------------------------------------------------------------------------------------------------------------------
template <typename VT, int Cap>
bool yaget::comp::db::PersistentObserverComponent<VT, Cap>::SetStorage(const auto& newStorage)
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
