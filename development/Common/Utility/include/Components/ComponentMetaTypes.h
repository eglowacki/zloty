//////////////////////////////////////////////////////////////////////
// ComponentMetaTypes.h
//
//  Copyright 11/5/2026 Edgar Glowacki
//
//  Maintained by: Edgar
//
//  NOTES:
//      Helper templates and utilities to help with 
//      meta programming for components and coordinators
//
//
//  #include "Components/ComponentMetaTypes.h"
//
//////////////////////////////////////////////////////////////////////
//! \file
#pragma once

#include "YagetCore.h"
#include "Meta/CompilerAlgo.h"

namespace yaget::comp::metatypes
{
    //----------------------------------------------------------------------------------------------------------------------------------
    template <typename T>
    concept has_coordinatorset = requires { typename T::CoordinatorSet_t; };

}
