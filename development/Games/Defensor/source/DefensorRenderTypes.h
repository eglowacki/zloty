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
        enum class ValidFields : uint8_t
        {
            None        = 0,
            Matrix      = 1 << 0,
            AssetGuid   = 1 << 1,
        };

        comp::Id_t mId;
        float mMatrix[16];
        unsigned char mAssetGuid[16];
    };  

    // NOTE(edgar): This is just a placeholder to show that we can serialize data from game thread into buffer 
    // and then consume it in render thread. We will need to fill this in with actual data and logic to serialize/deserialize
    inline size_t Serialize(const EntityState& entityState, io::BufferView& buffer)
    {
        memcpy(io::cast_data<char>(buffer), &entityState.mId, sizeof(entityState.mId));
        return 0;
    }
    //--------------------------------------------------------------------------

}
