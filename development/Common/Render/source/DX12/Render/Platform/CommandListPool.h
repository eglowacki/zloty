/////////////////////////////////////////////////////////////////////////
// CommandListPool.h
//
//  Copyright 09/03/2022 Edgar Glowacki.
//
// NOTES:
//      Provides pool for mananaging and re-using command lists
//
// #include "Render/Platform/CommandListPool.h"
//
/////////////////////////////////////////////////////////////////////////
//! \file

#pragma once

#include "Render/RenderCore.h"
#include "Render/Platform/CommandQueue.h"

#include <queue>


struct ID3D12CommandAllocator;
struct ID3D12GraphicsCommandList4;
struct ID3D12Device;


namespace yaget::render::platform
{
    //-------------------------------------------------------------------------------------------------
    class CommandListPool
    {
    public:
        CommandListPool(ID3D12Device* device, uint32_t numCommands);
        ~CommandListPool();

        struct Handle
        {
            Handle(CommandListPool& commandPool, ComPtr<ID3D12GraphicsCommandList4> commandList, CommandQueue::Type type);
            ~Handle();

            ID3D12GraphicsCommandList4* GetCommandList() const { return mCommandList.Get(); }

        private:
            CommandListPool& mCommandPool;
            ComPtr<ID3D12GraphicsCommandList4> mCommandList;
            CommandQueue::Type mType;
        };

        Handle GetCommandList(CommandQueue::Type type, ID3D12CommandAllocator* commandAllocator);

        using CommandsList = std::queue<ComPtr<ID3D12GraphicsCommandList4>>;

    private:
        void FreeUsed(ComPtr<ID3D12GraphicsCommandList4> commandList, CommandQueue::Type type);

        std::map<CommandQueue::Type, CommandsList> mFreeCommandsList;
    };

} // namespace yaget::render::platform

