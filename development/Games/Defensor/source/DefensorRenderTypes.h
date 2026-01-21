///////////////////////////////////////////////////////////////////////
// DefensorRenderTypes.h
//
//  Copyright 06/26/2024 Edgar Glowacki.
//
//  Maintained by: Edgar
//
//  NOTES:
//      Specific render types needed for Defensor game
//
//
//  #include "DefensorRenderTypes.h"
//
//////////////////////////////////////////////////////////////////////
//! \file
#pragma once

#include "Components/ComponentTypes.h"

namespace defensor::render
{
    using namespace yaget;

    //--------------------------------------------------------------------------
    // used in entity scene composition for renderer. This is filled in
    // from game thread and comsumed by render thread
    struct EntityState
    {
        yaget::comp::Id_t mId = comp::INVALID_ID;
        float mMatrix[16] = {};
        io::Tag mAsset;
    };  
    //--------------------------------------------------------------------------

}
