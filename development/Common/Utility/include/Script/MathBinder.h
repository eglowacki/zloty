///////////////////////////////////////////////////////////////////////
// MathBinder.h
//
//  Copyright 11/2/2026 Edgar Glowacki.
//
//  Maintained by: Edgar
//
//  NOTES:
//      Provides binding of math functions and utilities
//      
//
//  #include "Script/MathBinder.h"
//
//////////////////////////////////////////////////////////////////////
//! \file
#pragma once

#include "YagetCore.h"

namespace sol
{
    class state;
}

namespace yaget::script
{
    //----------------------------------------------------------------------------------------------------------------------------------
    void BindMath(sol::state& luaState, const std::string& luaNamespace);
    
}
