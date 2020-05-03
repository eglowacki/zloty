///////////////////////////////////////////////////////////////////////
// AtomicValue.h
//
//  Copyright 11/4/2006 Edgar Glowacki.
//
//  Maintained by: Edgar
//
//  NOTES:
//      Template class for updating value of type T thread safe
//
//
//  #include "Synchronization/AtomicValue.h"
//
//////////////////////////////////////////////////////////////////////
//! \file

#ifndef SYNCHRONIZATION_ATOMIC_VALUE_H
#define SYNCHRONIZATION_ATOMIC_VALUE_H
#pragma once

#include "Base.h"
#if 0
#include <wx/thread.h>

namespace eg
{
    /*!
    This is used to lock mostly primitive types for reading and writing to it
    Usage:
    \code
    AtomicValue<int32_t> aValue;
    \endcode
    */
    template <typename T>
    class AtomicValue
    {
    public:
        AtomicValue() : mValue(0)
        {
        }

        AtomicValue(T v) : mValue(v)
        {
        }

        operator const T() const
        {
            wxCriticalSectionLocker lock(mCS);
            return mValue;
        }

        T operator=(T n)
        {
            wxCriticalSectionLocker lock(mCS);
            mValue = n;
            return n;
        }

        T operator++()
        {
            wxCriticalSectionLocker lock(mCS);
            return ++mValue;
        }

        T operator++(int)
        {
            wxCriticalSectionLocker lock(mCS);
            return mValue++;
        }

        T operator--()
        {
            wxCriticalSectionLocker lock(mCS);
            return --mValue;
        }

        T operator--(int)
        {
            wxCriticalSectionLocker lock(mCS);
            return mValue--;
        }

    private:
        volatile T mValue;
        mutable wxCriticalSection mCS;
    };

} // namespace eg

#endif // 0
#endif // SYNCHRONIZATION_ATOMIC_VALUE_H

