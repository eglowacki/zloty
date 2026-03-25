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
#include "Render/RenderStringHelpers.h"
#include "Core/ErrorHandlers.h"
#include "magic_enum/magic_enum.hpp"

#include <d3dx12.h>


namespace yaget::render
{
    inline D3D12_COMMAND_LIST_TYPE ConvertCommandQueueType(commands::Type type)
    {
        using namespace yaget::render::commands;
        D3D12_COMMAND_LIST_TYPE dx12Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

        switch (type)
        {
        case Type::Direct:
            dx12Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
            break;

        case Type::Compute:
            dx12Type = D3D12_COMMAND_LIST_TYPE_COMPUTE;
            break;

        case Type::Copy:
            dx12Type = D3D12_COMMAND_LIST_TYPE_COPY;
            break;

        default:
            error_handlers::Throw("DEVI", std::format("Invalid Command Type Queue: {}.", magic_enum::enum_name(type)));
        }

        return dx12Type;
    }

} // namespace yaget::render


