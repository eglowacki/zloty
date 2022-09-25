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
struct ID3D12GraphicsCommandList4;

namespace yaget::render::platform
{
    //-------------------------------------------------------------------------------------------------
    class CommandQueue
    {
    public:
        enum class Type { Direct, Compute, Copy, End };
        
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
    private:
        // make it visible to CQ struct and keep it private
        struct CommandQueueData
        {
            CommandQueueData(ID3D12Device* device, CommandQueue::Type type);
            CommandQueueData() = default;
            ~CommandQueueData();

            CommandQueueData(CommandQueueData&& other) noexcept;
            CommandQueueData& operator=(CommandQueueData&& other) noexcept;

            uint64_t Signal();
            void Wait(uint64_t signalValue) const;
            void Flush();

            ComPtr<ID3D12CommandQueue> mCommandQueue;
            ComPtr<ID3D12Fence1> mFence;
            HANDLE mFenceEvent{ nullptr };
            std::atomic_uint64_t mFenceValue{ 0 };
        };

    public:
        CommandQueues(ID3D12Device* device);
        ~CommandQueues();

        void Reset();

        struct CQ : private yaget::Noncopyable<CQ>
        {
            CQ(CommandQueueData& cqData, bool finished);

            uint64_t Signal();
            void Wait(uint64_t signalValue) const;

            void Execute(ID3D12GraphicsCommandList4* commandList);
            void Execute(std::vector<ID3D12GraphicsCommandList4*> commands);

            ID3D12CommandQueue* GetCommandQueue() const;

        private:
            CommandQueueData& mCommandQueueData;
        };

        // Get command queue for the specific type. If finished is true
        // then return that command queue that finished with all commands.
        CQ GetCQ(CommandQueue::Type type, bool finished);

    private:
        std::map<CommandQueue::Type, CommandQueueData> mCommandQueues;
    };

} // namespace yaget::render::platform
