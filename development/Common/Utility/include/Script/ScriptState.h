///////////////////////////////////////////////////////////////////////
// ScriptState.h
//
//  Copyright 10/21/2026 Edgar Glowacki.
//
//  Maintained by: Edgar
//
//  NOTES:
//      Provides Lua state object
//      
//
//  #include "Script/ScriptState.h"
//
//////////////////////////////////////////////////////////////////////
//! \file
#pragma once

#include "YagetCore.h"
#include "sol/sol.hpp"


namespace yaget::script
{
    //--------------------------------------------------------------------------------------------------
    sol::state GetState();

}
