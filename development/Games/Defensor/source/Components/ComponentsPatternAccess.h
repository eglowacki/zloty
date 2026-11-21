///////////////////////////////////////////////////////////////////////
// ComponentsPatternAccess.h
//
//  Copyright 11/17/2026 Edgar Glowacki.
//
//  Maintained by: Edgar
//
//  NOTES:
//      Provides access to get type for a specific 
//      component tuple combination
//      
//
//  #include "Components/ComponentsPatternAccess.h"
//
//////////////////////////////////////////////////////////////////////
//! \file
#pragma once

#include "App/AppUtilities.h"
#include "generated\ComponentsPatternFile.cpp"

namespace yaget::comp::patterns
{

    void ResolveValue(size_t index, auto callback)
    {
        index;
        callback;

        FindValue(index, callback);
    }
    
}
