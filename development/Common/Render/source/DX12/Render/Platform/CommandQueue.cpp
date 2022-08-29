#include "CommandQueue.h"
#include "App/AppUtilities.h"
#include "Render/Platform/DeviceDebugger.h"
#include <d3d12.h>


//-------------------------------------------------------------------------------------------------
yaget::render::platform::CommandQueue::CommandQueue(ID3D12Device* device, Type type)
{
    D3D12_COMMAND_QUEUE_DESC queueDesc = {};
    queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    queueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
    queueDesc.NodeMask = 0;

    switch (type)
    {
    case Type::Direct:
        queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
        break;

    case Type::Compute:
        queueDesc.Type = D3D12_COMMAND_LIST_TYPE_COMPUTE;
        break;

    case Type::Copy:
        queueDesc.Type = D3D12_COMMAND_LIST_TYPE_COPY;
        break;
    }

    HRESULT hr = device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&mCommandQueue));
    YAGET_UTIL_THROW_ON_RROR(hr, "Could not create DX12 Command Queue");

    YAGET_RENDER_SET_DEBUG_NAME(mCommandQueue.Get(), "Yaget CommandQueue");

    hr = device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&mFence));
    YAGET_UTIL_THROW_ON_RROR(hr, "Could not create DX12 Fence");

    YAGET_RENDER_SET_DEBUG_NAME(mFence.Get(), "Yaget Fence");
    //mFence->AddRef(); // testing leak report

    mFenceEvent = ::CreateEvent(nullptr, FALSE, FALSE, nullptr);
    YAGET_UTIL_THROW_ON_RROR(mFenceEvent, "Could not create Event");

    YLOG_INFO("DEVI", "Command Queue created with Type: '%d'.", type);
}


//-------------------------------------------------------------------------------------------------
yaget::render::platform::CommandQueue::~CommandQueue() = default;


//-------------------------------------------------------------------------------------------------
void yaget::render::platform::CommandQueue::Execute()
{
}


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
