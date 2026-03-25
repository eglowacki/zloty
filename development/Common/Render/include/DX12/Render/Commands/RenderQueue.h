/////////////////////////////////////////////////////////////////////////
// RenderQueue.h
//
//  Copyright 03/24/2026 Edgar Glowacki.
//
// NOTES:
//      This handles command queues
//
// #include "Render/Commands/RenderQueue.h"
//
/////////////////////////////////////////////////////////////////////////
//! \file

#pragma once

#include "Render/RenderCore.h"
#include "Render/Commands/RenderCommandTypes.h"

struct ID3D12Device;
struct ID3D12CommandQueue;
struct ID3D12Fence;


namespace yaget::render::commands
{
    class CommandList;

    // create one queue per Queue::Type
    // NOTE(eg) do we need to create several queues for number of buffers and/or threads?
    //-------------------------------------------------------------------------------------------------
    // The basic render loop is one D3D12_COMMAND_LIST_TYPE_DIRECT Command Queue, 
    // one Command Allocator per back-buffer swap chain (2 or 3), 
    // and each frame has one Command List (reset from the appropriate command allocator) for graphics drawing.
    // We only need one Queue per commandType
    class Queue
    {
    public:
        Queue(ID3D12Device* device, Type commandType);
        ~Queue();

        ID3D12CommandQueue* GetDeviceCommandQueue() const;

        bool IsFenceComplete(uint64_t fenceValue);
        void InsertWait(uint64_t fenceValue);
        void InsertWaitForQueueFence(const Queue* otherQueue, uint64_t fenceValue) const;
        void InsertWaitForQueue(const Queue* otherQueue) const;

        size_t Signal();

        void WaitForFenceCPUBlocking(uint64_t fenceValue);
        void WaitForIdle();

        uint64_t PollCurrentFenceValue();
        uint64_t GetLastCompletedFence() const;
        uint64_t GetNextFenceValue() const;
        ID3D12Fence* GetFence() const;

        void ExecuteCommandList(CommandList* commandList);
        using CommandLists = std::vector<CommandList*>;
        void ExecuteCommandLists(const CommandLists& commandLists);
        
    private:
        Type mQueueType;
        ComPtr<ID3D12CommandQueue> mCommandQueue;

        std::mutex mFenceMutex;
        std::mutex mEventMutex;

        ComPtr<ID3D12Fence> mFence;
        uint64_t mNextFenceValue;
        uint64_t mLastCompletedFenceValue;
        HANDLE mFenceEventHandle;
    };
    
                                                                                                            
    //-------------------------------------------------------------------------------------------------
    class QueueStorage
    {
    public:
        QueueStorage(ID3D12Device* device);
        ~QueueStorage();

        Queue* GetQueue(Type commandType);

        bool IsFenceComplete(uint64_t fenceValue);
        void WaitForFenceCPUBlocking(uint64_t fenceValue);
        void WaitForAllIdle();

    private:
        using QueueMap = std::map<Type, std::unique_ptr<Queue>>;
        QueueMap mQueueMap;
    };

}
