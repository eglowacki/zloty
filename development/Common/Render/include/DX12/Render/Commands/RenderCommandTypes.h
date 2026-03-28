/////////////////////////////////////////////////////////////////////////
// RenderCommandTypes.h
//
//  Copyright 03/24/2026 Edgar Glowacki.
//
// NOTES:
//      This provides common data for render commands
//
// #include "Render/Commands/RenderCommandTypes.h"
//
/////////////////////////////////////////////////////////////////////////
//! \file

#pragma once

#include "Render/RenderCore.h"

namespace yaget::render::commands
{
    // We prime the fence with the queue type shifted by 56. 
    // I've seen this trick in a number of samples and have found it pretty handy for lazy queue lookups.
    // The idea is that all we need is the fence value itself to know which queue type it came from.
    static constexpr uint64_t QueueTypeOffset = 56;

    enum class Type : uint32_t
    {
        Direct = 0,     // D3D12_COMMAND_LIST_TYPE_DIRECT
        Compute = 2,    // D3D12_COMMAND_LIST_TYPE_COMPUTE
        Copy = 3,       // D3D12_COMMAND_LIST_TYPE_COPY
        Max             // used to create an array of last fence values in Device
    };


}