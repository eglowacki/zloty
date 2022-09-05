#include "Render/Platform/CommandQueue.h"
#include "App/AppUtilities.h"
#include "Render/Platform/DeviceDebugger.h"
#include "Render/RenderStringHelpers.h"

#include <d3d12.h>

namespace
{
    yaget::render::ComPtr<ID3D12CommandQueue> CreateCommandQueue(ID3D12Device* device, yaget::render::platform::CommandQueue::Type type)
    {
        using namespace yaget;

        D3D12_COMMAND_QUEUE_DESC queueDesc = {};
        queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
        queueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
        queueDesc.NodeMask = 0;

        switch (type)
        {
        case render::platform::CommandQueue::Type::Direct:
            queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
            break;

        case render::platform::CommandQueue::Type::Compute:
            queueDesc.Type = D3D12_COMMAND_LIST_TYPE_COMPUTE;
            break;

        case render::platform::CommandQueue::Type::Copy:
            queueDesc.Type = D3D12_COMMAND_LIST_TYPE_COPY;
            break;
        }

        render::ComPtr<ID3D12CommandQueue> commandQueue;
        HRESULT hr = device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&commandQueue));
        YAGET_UTIL_THROW_ON_RROR(hr, fmt::format("Could not create DX12 Command Queue for type: {}.", conv::Convertor<render::platform::CommandQueue::Type>::ToString(type)));

        YAGET_RENDER_SET_DEBUG_NAME(commandQueue.Get(), fmt::format("Yaget CommandQueue-{}", conv::Convertor<render::platform::CommandQueue::Type>::ToString(type)));

        return commandQueue;
    }
} // namespace


//-------------------------------------------------------------------------------------------------
yaget::render::platform::CommandQueue::CommandQueue(ID3D12Device* device, Type type)
{
    mCommandQueue = CreateCommandQueue(device, type);

    HRESULT hr = device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&mFence));
    YAGET_UTIL_THROW_ON_RROR(hr, "Could not create DX12 Fence");

    YAGET_RENDER_SET_DEBUG_NAME(mFence.Get(), "Yaget Fence");
    //mFence->AddRef(); // testing leak report

    mFenceEvent = ::CreateEvent(nullptr, FALSE, FALSE, nullptr);
    YAGET_UTIL_THROW_ON_RROR(mFenceEvent, "Could not create Event");

    YLOG_INFO("DEVI", "Command Queue created with Type: '%s'.", yaget::conv::Convertor<Type>::ToString(type).c_str());
}


//-------------------------------------------------------------------------------------------------
yaget::render::platform::CommandQueue::~CommandQueue() = default;


//-------------------------------------------------------------------------------------------------
void yaget::render::platform::CommandQueue::Flush()
{
    uint64_t fenceValueForSignal = Signal();
    WaitForFenceValue(fenceValueForSignal);
}


//-------------------------------------------------------------------------------------------------
uint64_t yaget::render::platform::CommandQueue::Signal()
{
    const uint64_t fenceValueForSignal = ++mFenceValue;

    HRESULT hr = mCommandQueue->Signal(mFence.Get(), fenceValueForSignal);
    YAGET_UTIL_THROW_ON_RROR(hr, "Could not signal DX12 Command Queue");

    return fenceValueForSignal;
}


//-------------------------------------------------------------------------------------------------
void yaget::render::platform::CommandQueue::WaitForFenceValue(uint64_t fenceValue) const
{
    if (mFence->GetCompletedValue() < fenceValue)
    {
        HRESULT hr = mFence->SetEventOnCompletion(fenceValue, mFenceEvent);
        YAGET_UTIL_THROW_ON_RROR(hr, "Could not set DX12 Event On Completion");

        ::WaitForSingleObject(mFenceEvent, INFINITE);
    }
}


//-------------------------------------------------------------------------------------------------
const Microsoft::WRL::ComPtr<struct ID3D12CommandQueue>& yaget::render::platform::CommandQueue::GetCommandQueue() const
{
    return mCommandQueue;
}


//-------------------------------------------------------------------------------------------------
yaget::render::platform::CommandQueuesSet::CommandQueuesSet(ID3D12Device* device)
{
    mCommandQueues[CommandQueue::Type::Direct] = CreateCommandQueue(device, CommandQueue::Type::Direct);
    mCommandQueues[CommandQueue::Type::Compute] = CreateCommandQueue(device, CommandQueue::Type::Compute);
    mCommandQueues[CommandQueue::Type::Copy] = CreateCommandQueue(device, CommandQueue::Type::Copy);
}


//-------------------------------------------------------------------------------------------------
yaget::render::platform::CommandQueuesSet::~CommandQueuesSet()
{
}


//-------------------------------------------------------------------------------------------------
ID3D12CommandQueue* yaget::render::platform::CommandQueuesSet::GetCommandQueue(CommandQueue::Type type) const
{
    YAGET_UTIL_THROW_ASSERT("DEVI", mCommandQueues.find(type) != mCommandQueues.end(), fmt::format("Invalid command queue type: {}.", conv::Convertor<CommandQueue::Type>::ToString(type)));

    return mCommandQueues.find(type)->second.Get();
}
