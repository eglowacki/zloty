#include "magic_enum/magic_enum.hpp"
#include "Render/Commands/RenderCommandList.h"
#include "Render/Commands/RenderQueue.h"
#include "Render/EnumConversion.h"
#include "Render/Platform/D3D12MemAlloc.h"
#include "Render/Platform/DeviceDebugger.h"

#include <d3dx12.h>
#include <ranges>



namespace
{
    //-------------------------------------------------------------------------------------------------
    yaget::render::ComPtr<ID3D12CommandQueue> CreateCommandQueue(ID3D12Device* device, yaget::render::commands::Type type)
    {
        using namespace yaget;

        D3D12_COMMAND_QUEUE_DESC queueDesc = {};
        queueDesc.Type = render::ConvertCommandQueueType(type);
        queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
        queueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
        queueDesc.NodeMask = 0;

        render::ComPtr<ID3D12CommandQueue> commandQueue;
        HRESULT hr = device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&commandQueue));
        error_handlers::ThrowOnError(hr, std::format("Could not create DX12 Command Queue for type: {}.", magic_enum::enum_name(type)));
        YAGET_RENDER_SET_DEBUG_NAME(commandQueue.Get(), std::format("CommandQueue-{}", magic_enum::enum_name(type)));

        return commandQueue;
    }


    //-------------------------------------------------------------------------------------------------
    yaget::render::ComPtr<ID3D12Fence1> CreateCommandQueueFence(ID3D12Device* device, yaget::render::commands::Type type)
    {
        using namespace yaget;

        render::ComPtr<ID3D12Fence1> fence;
        HRESULT hr = device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence));
        error_handlers::ThrowOnError(hr, std::format("Could not create DX12 CommandQueue.Fence for type: {}.", magic_enum::enum_name(type)));
        YAGET_RENDER_SET_DEBUG_NAME(fence.Get(), std::format("CommandQueue.Fence-{}", magic_enum::enum_name(type)));

        return fence;
    }

}


//-------------------------------------------------------------------------------------------------
yaget::render::commands::Queue::Queue(ID3D12Device* device, Type commandType, FenceValues& fenceValues)
    : mQueueType(commandType)
    , mCommandQueue{ CreateCommandQueue(device, mQueueType) }
    , mFence{ CreateCommandQueueFence(device, mQueueType) }
    , mFenceValues{ fenceValues }
    , mFenceEventHandle{ nullptr }
{
    mFenceValues.Initialize(mQueueType);

    HRESULT hr = mFence->Signal(mFenceValues.mLastCompletedFenceValue);
    error_handlers::ThrowOnError(hr, "Could not Signal DX12 CommandQueue.Fence");
 
    mFenceEventHandle = CreateEventEx(nullptr, L"CommandQueue.Event", false, EVENT_ALL_ACCESS);
    error_handlers::ThrowOnError(mFenceEventHandle != INVALID_HANDLE_VALUE, "Did not create CommandQueue.Event");
}


//-------------------------------------------------------------------------------------------------
yaget::render::commands::Queue::~Queue()
{
    CloseHandle(mFenceEventHandle);
}


//-------------------------------------------------------------------------------------------------
bool yaget::render::commands::Queue::IsFenceComplete(uint64_t fenceValue)
{
    if (fenceValue > mFenceValues.mLastCompletedFenceValue)
    {
        PollCurrentFenceValue();
    }

    return fenceValue <= mFenceValues.mLastCompletedFenceValue;
}


//-------------------------------------------------------------------------------------------------
void yaget::render::commands::Queue::InsertWait(uint64_t fenceValue)
{
    // Use this method to set a fence value from the GPU side.
    mCommandQueue->Wait(mFence.Get(), fenceValue);
}


//-------------------------------------------------------------------------------------------------
void yaget::render::commands::Queue::InsertWaitForQueueFence(const Queue* otherQueue, uint64_t fenceValue) const
{
    mCommandQueue->Wait(otherQueue->GetFence(), fenceValue);
}



//-------------------------------------------------------------------------------------------------
void yaget::render::commands::Queue::InsertWaitForQueue(const Queue* otherQueue) const
{
    InsertWaitForQueueFence(otherQueue, otherQueue->GetNextFenceValue() - 1);
}


//-------------------------------------------------------------------------------------------------
size_t yaget::render::commands::Queue::Signal()
{
    std::lock_guard lockGuard(mFenceMutex);
 
    mCommandQueue->Signal(mFence.Get(), mFenceValues.mNextFenceValue);
 
    return mFenceValues.mNextFenceValue++;
}


//-------------------------------------------------------------------------------------------------
void yaget::render::commands::Queue::WaitForFenceCPUBlocking(uint64_t fenceValue)
{
    if (IsFenceComplete(fenceValue))
    {
        return;
    }
 
    std::lock_guard lockGuard(mEventMutex);

    HRESULT hr = mFence->SetEventOnCompletion(fenceValue, mFenceEventHandle);
    error_handlers::ThrowOnError(hr, "Could not set Fence Event On Completion");

    WaitForSingleObjectEx(mFenceEventHandle, INFINITE, false);
    mFenceValues.mLastCompletedFenceValue = fenceValue;
}


//-------------------------------------------------------------------------------------------------
void yaget::render::commands::Queue::WaitForIdle()
{
    WaitForFenceCPUBlocking(mFenceValues.mNextFenceValue - 1);
}


//-------------------------------------------------------------------------------------------------
ID3D12CommandQueue* yaget::render::commands::Queue::GetDeviceCommandQueue() const
{
    return mCommandQueue.Get();
}


//-------------------------------------------------------------------------------------------------
uint64_t yaget::render::commands::Queue::PollCurrentFenceValue()
{
    return mFenceValues.PollCurrentFenceValue(mFence->GetCompletedValue());
}


//-------------------------------------------------------------------------------------------------
uint64_t yaget::render::commands::Queue::GetLastCompletedFence() const
{
    return mFenceValues.mLastCompletedFenceValue;
}


//-------------------------------------------------------------------------------------------------
uint64_t yaget::render::commands::Queue::GetNextFenceValue() const
{
    return mFenceValues.mNextFenceValue;
}


//-------------------------------------------------------------------------------------------------
ID3D12Fence* yaget::render::commands::Queue::GetFence() const
{
    return mFence.Get();
}


//-------------------------------------------------------------------------------------------------
void yaget::render::commands::Queue::ExecuteCommandList(CommandList* commandList)
{
    ExecuteCommandLists({ commandList });
}


//-------------------------------------------------------------------------------------------------
void yaget::render::commands::Queue::ExecuteCommandLists(const CommandLists& commandLists)
{
    if (commandLists.empty())
    {
        return;
    }

    for (const auto& commandList : commandLists)
    {
        commandList->Close();
    }

    std::vector<ID3D12CommandList*> deviceCommandList = commandLists | std::ranges::views::transform([this](const auto& command)
    {
        return static_cast<ID3D12CommandList*>(command->GetDeviceCommandList());
    }) | std::ranges::to<std::vector>();

    mCommandQueue->ExecuteCommandLists(static_cast<uint32_t>(deviceCommandList.size()), &deviceCommandList[0]);
}


//-------------------------------------------------------------------------------------------------
yaget::render::commands::QueueStorage::QueueStorage(ID3D12Device* device, QueueFenceValues& queueFenceValues)
{
    mQueueMap[Type::Direct] = std::make_unique<Queue>(device, Type::Direct, queueFenceValues.mFenceValues[static_cast<uint32_t>(Type::Direct)]);
    mQueueMap[Type::Compute] = std::make_unique<Queue>(device, Type::Compute, queueFenceValues.mFenceValues[static_cast<uint32_t>(Type::Compute)]);
    mQueueMap[Type::Copy] = std::make_unique<Queue>(device, Type::Copy, queueFenceValues.mFenceValues[static_cast<uint32_t>(Type::Copy)]);
}


//-------------------------------------------------------------------------------------------------
yaget::render::commands::QueueStorage::~QueueStorage() = default;


//-------------------------------------------------------------------------------------------------
yaget::render::commands::Queue* yaget::render::commands::QueueStorage::GetQueue(Type commandType)
{
    YAGET_ASSERT(mQueueMap.contains(commandType), std::format("Invalid command queue type: '{}'.", magic_enum::enum_name(commandType)).c_str());

    return mQueueMap[commandType].get();
}


//-------------------------------------------------------------------------------------------------
bool yaget::render::commands::QueueStorage::IsFenceComplete(uint64_t fenceValue)
{
    Queue *commandQueue = GetQueue(static_cast<Type>(fenceValue >> QueueTypeOffset));
    return commandQueue->IsFenceComplete(fenceValue);
}


//-------------------------------------------------------------------------------------------------
void yaget::render::commands::QueueStorage::WaitForFenceCPUBlocking(uint64_t fenceValue)
{
	Queue *commandQueue = GetQueue(static_cast<Type>(fenceValue >> QueueTypeOffset));
	commandQueue->WaitForFenceCPUBlocking(fenceValue);
}


//-------------------------------------------------------------------------------------------------
void yaget::render::commands::QueueStorage::WaitForAllIdle()
{
    for (auto& queue : mQueueMap | std::views::values)
    {
        queue->WaitForIdle();
    }
}

