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

#ifndef YAGET_STL_HELPER_H
#define YAGET_STL_HELPER_H
#pragma once

#include <Base.h>

namespace yaget
{
    template <std::size_t N>
    struct type_of_size
    {
        typedef char type[N];
    };

    template <typename T, std::size_t Size>
    typename type_of_size<Size>::type& sizeof_array_helper(T(&)[Size]);

    #define sizeof_array(pArray) sizeof(yaget::sizeof_array_helper(pArray))

} // namespace yaget

#endif // YAGET_STL_HELPER_H
