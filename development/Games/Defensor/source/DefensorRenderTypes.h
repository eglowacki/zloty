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
    // from game thread and consumed by render thread
    struct EntityState
    {
        comp::Id_t mId;
        float mMatrix[16];
        unsigned char mAssetGuid[16];
    };  

    // NOTE(edgar): This is just a placeholder to show that we can serialize data from game thread into buffer 
    // and then consume it in render thread. We will need to fill this in with actual data and logic to serialize/deserialize
    inline size_t Serialize(const EntityState& entityState, io::BufferView& buffer)
    {
        entityState;
        buffer;

        int z =0;
        z;

        return 0;
    }
    //--------------------------------------------------------------------------

}
