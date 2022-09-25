/////////////////////////////////////////////////////////////////////////
// EnumConversion.h
//
//  Copyright 09/25/2023 Edgar Glowacki.
//
// NOTES:
//      Utility functions to convert from our enum types to DX types
//
// #include "Render/EnumConversion.h"
//
/////////////////////////////////////////////////////////////////////////
//! \file

#pragma once

#include "Render/RenderCore.h"
#include "Render/Platform/CommandQueue.h"
#include "Render/RenderStringHelpers.h"

#include <d3dx12.h>


namespace yaget::render
{
    inline D3D12_COMMAND_LIST_TYPE ConvertCommandQueueType(yaget::render::platform::CommandQueue::Type cqType)
    {
        using namespace yaget::render::platform;
        D3D12_COMMAND_LIST_TYPE type = D3D12_COMMAND_LIST_TYPE_DIRECT;

        switch (cqType)
        {
        case CommandQueue::Type::Direct:
            type = D3D12_COMMAND_LIST_TYPE_DIRECT;
            break;

        case CommandQueue::Type::Compute:
            type = D3D12_COMMAND_LIST_TYPE_COMPUTE;
            break;

        case CommandQueue::Type::Copy:
            type = D3D12_COMMAND_LIST_TYPE_COPY;
            break;

        default:
            YAGET_UTIL_THROW("DEVI", fmt::format("Invalid Command Type Queue: {}.", yaget::conv::Convertor<CommandQueue::Type>::ToString(cqType)));
        }

        return type;
    }

} // namespace yaget::render


