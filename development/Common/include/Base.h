///////////////////////////////////////////////////////////////////////
// Base.h
//
//  Copyright 10/1/2005 Edgar Glowacki.
//
//  Maintained by: Edgar
//
//  NOTES:
//      Basic #define and typedef, which is included in all the Muck source files
//
//
//  #include "Base.h"
//
//////////////////////////////////////////////////////////////////////
//! \file
#pragma once

//! Entire YAGET engine is encapsulated in this namespace
namespace yaget
{}

/*!
\mainpage YAGET Engine
This is 3D engine (Yet Another Game Engine Technology)
*/

#include <cstddef>
#include <cstdint>
#include <string>

#if defined(YAGET_NEW_ALLOCATOR_ENABLED) && (YAGET_NEW_ALLOCATOR_ENABLED == 1)
    #ifdef YAGET_DEBUG
        #define _CRTDBG_MAP_ALLOC
    #endif // YAGET_DEBUG

    #include <stdlib.h>
    #include <crtdbg.h>
#endif // YAGET_NEW_ALLOCATOR_ENABLED

// Used in generating unique name during compile. Used in macros that construct variable names
#define YAGET_STRINGIZE_P(x) #x
#define YAGET_STRINGIZE(x) YAGET_STRINGIZE_P(x)

#define YAGET_PP_CAT(a, b) YAGET_PP_CAT_I(a, b)
#define YAGET_PP_CAT_I(a, b) YAGET_PP_CAT_II(~, a ## b)
#define YAGET_PP_CAT_II(p, res) res

#define YAGET_UNIQUE_NAME(base) YAGET_PP_CAT(base, __COUNTER__)


// used in suppressing unused variables for variadic parameters in other macros, like LOG, ASSERT
#define YLOG_UNUSED1(a)                     (void)(a)
#define YLOG_UNUSED2(a,b)                   (void)(a),YLOG_UNUSED1(b)
#define YLOG_UNUSED3(a,b,c)                 (void)(a),YLOG_UNUSED2(b,c)
#define YLOG_UNUSED4(a,b,c,d)               (void)(a),YLOG_UNUSED3(b,c,d)
#define YLOG_UNUSED5(a,b,c,d,e)             (void)(a),YLOG_UNUSED4(b,c,d,e)
#define YLOG_UNUSED6(a,b,c,d,e,f)           (void)(a),YLOG_UNUSED5(b,c,d,e,f)
#define YLOG_UNUSED7(a,b,c,d,e,f,g)         (void)(a),YLOG_UNUSED6(b,c,d,e,f)
#define YLOG_UNUSED8(a,b,c,d,e,f,g,h)       (void)(a),YLOG_UNUSED7(b,c,d,e,f,g)
#define YLOG_UNUSED9(a,b,c,d,e,f,g,h,i)     (void)(a),YLOG_UNUSED8(b,c,d,e,f,g,h)
#define YLOG_UNUSED10(a,b,c,d,e,f,g,h,i,j)  (void)(a),YLOG_UNUSED9(b,c,d,e,f,g,h,i)

#define YLOG_VA_NUM_ARGS_IMPL(_1,_2,_3,_4,_5,_6,_7,_8,_9, N,...) N
#define YLOG_VA_NUM_ARGS(...)               YLOG_VA_NUM_ARGS_IMPL(__VA_ARGS__, 9, 8, 7, 6, 5, 4, 3, 2, 1)

#define YLOG_ALL_UNUSED_IMPL_(nargs)        YLOG_UNUSED ## nargs
#define YLOG_ALL_UNUSED_IMPL(nargs)         YLOG_ALL_UNUSED_IMPL_(nargs)
#define YLOG_ALL_UNUSED(...)                YLOG_ALL_UNUSED_IMPL( YLOG_VA_NUM_ARGS(__VA_ARGS__))(__VA_ARGS__ )


