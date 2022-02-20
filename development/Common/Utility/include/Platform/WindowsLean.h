///////////////////////////////////////////////////////////////////////
// WindowsLean.h
//
//  Copyright 10/1/2005 Edgar Glowacki.
//
//  Maintained by: Edgar
//
//  NOTES:
//      Include windows header with lean and mean defined. If dev platform
//      is not windows, then issue compile error
//
//
//  #include "Platform/WindowsLean.h"
//
//////////////////////////////////////////////////////////////////////
//! \file
#pragma once

#if defined(_WIN32)

    #if defined(NOMINMAX)
    #else
        #define NOMINMAX
    #endif

    #define WIN32_LEAN_AND_MEAN
        #include <Windows.h>
    #undef NOMINMAX

#else
    #error Expected Windows headers, but dev platform is not set, examine include file stack to see if platform dependent code is needed
#endif // _WIN32
