/////////////////////////////////////////////////////////////////////////
// safe_bool.h
//
//  Copyright 5/24/2009 Edgar Glowacki.
//
// NOTES:
//  Provides base class to add bool operator compiroson
//
// #include "safe_bool.h"
//
/////////////////////////////////////////////////////////////////////////
//! \file

#ifndef SAFE_BOOL_H
#define SAFE_BOOL_H
#pragma once

namespace eg
{
    /*
    \code
    class Testable_with_virtual : public safe_bool<>
    {
    protected:
        bool boolean_test() const
        {
            // Perform Boolean logic here
        }
    };


    class Testable_without_virtual : public safe_bool <Testable_without_virtual>
    {
    public:
        bool boolean_test() const
        {
            // Perform Boolean logic here
        }
    };
    \endcode
    */
    class safe_bool_base
    {
    protected:
    public:
        typedef void (safe_bool_base::*bool_type)() const;
        void this_type_does_not_support_comparisons() const {}

        safe_bool_base() {}
        safe_bool_base(const safe_bool_base&) {}
        safe_bool_base& operator=(const safe_bool_base&) {return *this;}
        ~safe_bool_base() {}
    };


    template <typename T = void>
    class safe_bool : public safe_bool_base
    {
    public:
        operator bool_type() const
        {
            return(static_cast<const T*>(this))->boolean_test() ? &safe_bool_base::this_type_does_not_support_comparisons : 0;
        }

    protected:
        ~safe_bool() {}
    };


    template<>
    class safe_bool<void> : public safe_bool_base
    {
    public:
        operator bool_type() const
        {
            return boolean_test() == true ? &safe_bool_base::this_type_does_not_support_comparisons : 0;
        }

    protected:
        virtual bool boolean_test() const = 0;
        virtual ~safe_bool() {}
    };

    template <typename T, typename U>
    void operator==(const safe_bool<T>& lhs, const safe_bool<U>& rhs)
    {
        lhs.this_type_does_not_support_comparisons();
        return false;
    }

    template <typename T,typename U>
    void operator!=(const safe_bool<T>& lhs, const safe_bool<U>& rhs)
    {
        lhs.this_type_does_not_support_comparisons();
        return false;
    }


} // namespace eg

#endif // SAFE_BOOL_H

