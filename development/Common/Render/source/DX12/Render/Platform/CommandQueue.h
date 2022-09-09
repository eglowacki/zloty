/////////////////////////////////////////////////////////////////////////
// CommandQueue.h
//
//  Copyright 07/03/2021 Edgar Glowacki.
//
// NOTES:
//      This handles command queue and fence
//
// #include "Render/Platform/CommandQueue.h"
//
/////////////////////////////////////////////////////////////////////////
//! \file

#pragma once

#include "Render/RenderCore.h"

struct ID3D12Device;
struct ID3D12CommandQueue;
struct ID3D12Fence1;

namespace yaget::render::platform
{
    //-------------------------------------------------------------------------------------------------
    class CommandQueue
    {
    public:
        enum class Type { Direct, Compute, Copy };
        
        CommandQueue(ID3D12Device* device, Type type);
        ~CommandQueue();

        void Flush();
        uint64_t Signal();
        void WaitForFenceValue(uint64_t fenceValue) const;

        const ComPtr<ID3D12CommandQueue>& GetCommandQueue() const;

    private:
        ComPtr<ID3D12CommandQueue> mCommandQueue;
        ComPtr<ID3D12Fence1> mFence;
        HANDLE mFenceEvent{ nullptr };
        std::atomic_uint64_t mFenceValue{ 0 };
    };
    

    //-------------------------------------------------------------------------------------------------
    // This class manages and exposes various types of Command Queues
    // This should replace the class above (CommandQueue)
    class CommandQueues
    {
    public:
        CommandQueues(ID3D12Device* device);
        ~CommandQueues();

        ID3D12CommandQueue* GetCommandQueue(CommandQueue::Type type) const;

    private:
        struct CommandQueueData
        {
            CommandQueueData(ID3D12Device* device, CommandQueue::Type type);
            CommandQueueData() = default;

            ComPtr<ID3D12CommandQueue> mCommandQueue;
            ComPtr<ID3D12Fence1> mFence;
            uint32_t mFenceValue = 0;
        };

        std::map<CommandQueue::Type, CommandQueueData> mCommandQueues;
    };

} // namespace yaget::render::platform
