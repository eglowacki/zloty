/////////////////////////////////////////////////////////////////////////
// Variables.h
//
//  Copyright 2/3/2018 Edgar Glowacki.
//
// NOTES:
//      Provides multi threaded support for reading and writing to data
//
//
// #include "ThreadModel/Variables.h"
//
/////////////////////////////////////////////////////////////////////////
//! \file
#pragma once

#include "YagetCore.h"
#include <mutex>


namespace yaget
{
    namespace mt
    {
        //! Provides mutexed access for reading and writing of user data. If you need only basic types, use std::atomics
        //! Usage:
        //!  struct Foo {};
        //!  Foo f1, f2;
        //!  Variable<Foo> varFoo;
        //!  varFoo = f1;
        //!  f2 = varFoo;
        //!  assert(f1 == f2);
        template<typename T>
        struct Variable : public Noncopyable<Variable<T>>
        {
            using Type = T;

            Variable() = default;

            template<typename... Args>
            Variable(Args&&... args) : mData(std::forward<Args>(args)...)
            {}

            T operator =(const T& v)
            {
                std::unique_lock<std::mutex> mutexLock(*mMutex);
                mData = v;
                return mData;
            }

            operator T() const
            {
                std::unique_lock<std::mutex> mutexLock(*mMutex);
                return mData;
            }

            T Swap(const T& v)
            {
                std::unique_lock<std::mutex> mutexLock(*mMutex);
                T oldData = mData;
                mData = v;

                return oldData;
            }

        private:
            T mData;
            mutable std::unique_ptr<std::mutex> mMutex = std::make_unique<std::mutex>();
        };

        
        //--------------------------------------------------------------------------------------------------
        // Provides specialized mt read and write for shared_ptr<T>
        // TODO: make this class specialized template from Variable
        template<typename T>
        struct SmartVariable : public Noncopyable<SmartVariable<T>>
        {
            using SmartType = std::shared_ptr<T>;

            // base interface to interact with
            SmartVariable() = default;

            template<typename... Args>
            SmartVariable(Args&&... args) : mData(std::make_shared<T>(std::forward<Args>(args)...))
            {}

            void operator =(const SmartType& value)
            {
                std::atomic_store(&mData, value);
            }

            operator SmartType() const
            {
                SmartType value = std::atomic_load(&mData);
                return value;
            }

            T GetValue() const
            {
                SmartType value = std::atomic_load(&mData);
                return value ? *value : T{};
            }

            bool empty() const
            {
                SmartType value = std::atomic_load(&mData);
                return value == SmartType();
            }

        private:
            SmartType mData;
        };

    } // namespace mt
} // namespace yaget


