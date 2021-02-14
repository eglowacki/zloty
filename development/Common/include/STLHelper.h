///////////////////////////////////////////////////////////////////////
// STLHelper.h
//
//  Copyright 11/6/2005 Edgar Glowacki.
//
//  Maintained by: Edgar
//
//  NOTES:
//
//
//  #include "STLHelper.h"
//
//////////////////////////////////////////////////////////////////////
//! \file

#pragma once

#include "YagetCore.h"
#include <unordered_set>

namespace yaget
{
    template <std::size_t N>
    struct type_of_size
    {
        typedef char type[N];
    };

    template <typename T, std::size_t Size>
    typename type_of_size<Size>::type& sizeof_array_helper(T(&)[Size]);

    // remove duplicate elements from container without changing the order
    template <typename T>
    void RemoveDuplicates(T& num)
    {
        std::unordered_set<T::value_type> set;
        std::size_t pos = 0;
        for (auto v : num)
        {
            if (set.insert(v).second)
            {
                num[pos++] = v;
            }
        }
        num.resize(pos);
    }

} // namespace yaget

#define sizeof_array(pArray) sizeof(yaget::sizeof_array_helper(pArray))
