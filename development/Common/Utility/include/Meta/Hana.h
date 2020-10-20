//////////////////////////////////////////////////////////////////////
// Hana.h
//
//  Copyright 6/30/2019 Edgar Glowacki
//
//  Maintained by: Edgar
//
//  NOTES:
//      Since VC++2019 (
//
//
//  #include "Meta/Hana.h"
//
//////////////////////////////////////////////////////////////////////
//! \file
#pragma once


namespace std
{
    template <class _Ty>
    struct is_literal_type : bool_constant<__is_literal_type(_Ty)> {
        // determine whether _Ty is a literal type
    };

}

#include <boost/hana.hpp>


