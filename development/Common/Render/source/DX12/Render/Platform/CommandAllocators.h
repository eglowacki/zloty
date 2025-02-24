/////////////////////////////////////////////////////////////////////////
// CommandAllocators.h
//
//  Copyright 09/04/2022 Edgar Glowacki.
//
// NOTES:
//      Provides holder for all command allocotors
//
// #include "Render/Platform/CommandAllocators.h"
//
/////////////////////////////////////////////////////////////////////////
//! \file

#pragma once

#include "Render/RenderCore.h"
#include "Render/Platform/CommandQueue.h"

//#include <queue>


struct ID3D12CommandAllocator;
struct ID3D12Device;


namespace yaget::render::platform
{
    //-------------------------------------------------------------------------------------------------
    class CommandAllocators
    {
    public:
        CommandAllocators(ID3D12Device* device, uint32_t numAllocators);
        ~CommandAllocators();

        ID3D12CommandAllocator* GetCommandAllocator(CommandQueue::Type type, uint32_t allocatorIndex) const;

        using AllocatorsList = std::vector<ComPtr<ID3D12CommandAllocator>>;

    private:
        std::map<CommandQueue::Type, AllocatorsList> mCommandAllocatorList;
    };

} // namespace yaget::render::platform

