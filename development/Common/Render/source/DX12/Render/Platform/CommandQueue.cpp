#include "Render/Platform/CommandQueue.h"
#include "App/AppUtilities.h"
#include "Render/Platform/DeviceDebugger.h"
#include "Render/RenderStringHelpers.h"
#include "Render/EnumConversion.h"
#include "fmt/format.h"

#include <type_traits>
#include <d3d12.h>

namespace
{
    yaget::render::ComPtr<ID3D12CommandQueue> CreateCommandQueue(ID3D12Device* device, yaget::render::platform::CommandQueue::Type cqType)
    {
        using namespace yaget;

        D3D12_COMMAND_QUEUE_DESC queueDesc = {};
        queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
        queueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
        queueDesc.NodeMask = 0;

        queueDesc.Type = yaget::render::ConvertCommandQueueType(cqType);

        render::ComPtr<ID3D12CommandQueue> commandQueue;
        HRESULT hr = device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&commandQueue));
        YAGET_UTIL_THROW_ON_RROR(hr, fmt::format("Could not create DX12 Command Queue for type: {}.", conv::Convertor<render::platform::CommandQueue::Type>::ToString(cqType)));

        YAGET_RENDER_SET_DEBUG_NAME(commandQueue.Get(), fmt::format("Yaget CommandQueue-{}", conv::Convertor<render::platform::CommandQueue::Type>::ToString(cqType)));

        return commandQueue;
    }

    yaget::render::ComPtr<ID3D12Fence1> CreateFence(ID3D12Device* device)
    {
        yaget::render::ComPtr<ID3D12Fence1> fence;

        HRESULT hr = device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence));
        YAGET_UTIL_THROW_ON_RROR(hr, "Could not create DX12 Fence");

        YAGET_RENDER_SET_DEBUG_NAME(fence.Get(), "Yaget Fence");

        return fence;
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
//-------------------------------------------------------------------------------------------------
yaget::render::platform::CommandQueues::CommandQueues(ID3D12Device* device)
{
    using CQIterator = yaget::meta::EnumIterator<CommandQueue::Type, CommandQueue::Type::Direct, CommandQueue::Type::End, false>;

    for (CommandQueue::Type i : CQIterator()) 
    {
        mCommandQueues[i] = CommandQueueData(device, i);
    }
}


//-------------------------------------------------------------------------------------------------
yaget::render::platform::CommandQueues::~CommandQueues() = default;


//-------------------------------------------------------------------------------------------------
yaget::render::platform::CommandQueues::CQ yaget::render::platform::CommandQueues::GetCQ(CommandQueue::Type type, bool finished)
{
    YAGET_ASSERT(mCommandQueues.find(type) != mCommandQueues.end(), "Invalid command queue type: %s.", conv::Convertor<CommandQueue::Type>::ToString(type).c_str());

    return CQ(mCommandQueues.find(type)->second, finished);
}


//-------------------------------------------------------------------------------------------------
void yaget::render::platform::CommandQueues::Reset()
{
    for (auto& [key, value] : mCommandQueues)
    {
        value.Flush();
    }
}


//-------------------------------------------------------------------------------------------------
yaget::render::platform::CommandQueues::CommandQueueData::CommandQueueData(ID3D12Device* device, CommandQueue::Type type)
    : mCommandQueue{ CreateCommandQueue(device, type) }
    , mFence{ CreateFence(device) }
    , mFenceEvent{ ::CreateEvent(nullptr, FALSE, FALSE, nullptr) }
    , mFenceValue{ 0 }
{
}


//-------------------------------------------------------------------------------------------------
yaget::render::platform::CommandQueues::CommandQueueData::~CommandQueueData()
{
    if (mCommandQueue)
    {
        Flush();
    }
}


//-------------------------------------------------------------------------------------------------
yaget::render::platform::CommandQueues::CommandQueueData::CommandQueueData(CommandQueueData&& other) noexcept
    : mCommandQueue{ std::move(other.mCommandQueue) }
    , mFence{ std::move(other.mFence) }
    , mFenceEvent{ std::move(other.mFenceEvent) }
    , mFenceValue{ other.mFenceValue.load() }
{
}


//-------------------------------------------------------------------------------------------------
yaget::render::platform::CommandQueues::CommandQueueData& yaget::render::platform::CommandQueues::CommandQueueData::operator=(CommandQueueData&& other) noexcept
{
    if (this != &other)
    {
        mCommandQueue = std::move(other.mCommandQueue);
        mFence = std::move(other.mFence);
        mFenceEvent = std::move(other.mFenceEvent);
        mFenceValue = other.mFenceValue.load();
    }

    return *this;
}


//-------------------------------------------------------------------------------------------------
uint64_t yaget::render::platform::CommandQueues::CommandQueueData::Signal()
{
    mFenceValue++;
    const uint64_t fenceValueForSignal = mFenceValue;

    HRESULT hr = mCommandQueue->Signal(mFence.Get(), fenceValueForSignal);
    YAGET_UTIL_THROW_ON_RROR(hr, "Could not signal DX12 Command Queue");

    return fenceValueForSignal;
}


//-------------------------------------------------------------------------------------------------
void yaget::render::platform::CommandQueues::CommandQueueData::Wait(uint64_t signalValue) const
{
    if (mFence->GetCompletedValue() < signalValue)
    {
        HRESULT hr = mFence->SetEventOnCompletion(signalValue, mFenceEvent);
        YAGET_UTIL_THROW_ON_RROR(hr, "Could not set DX12 Event On Completion");

        ::WaitForSingleObject(mFenceEvent, INFINITE);
    }
}


//-------------------------------------------------------------------------------------------------
void yaget::render::platform::CommandQueues::CommandQueueData::Flush()
{
    const auto signalValue = Signal();
    Wait(signalValue);
}


//-------------------------------------------------------------------------------------------------
yaget::render::platform::CommandQueues::CQ::CQ(CommandQueueData& cqData, bool finished)
    : mCommandQueueData(cqData)
{
    if (finished)
    {
        mCommandQueueData.Flush();
    }
}


//-------------------------------------------------------------------------------------------------
uint64_t yaget::render::platform::CommandQueues::CQ::Signal()
{
    const auto signalValue = mCommandQueueData.Signal();
    return signalValue;
}


//-------------------------------------------------------------------------------------------------
void yaget::render::platform::CommandQueues::CQ::Wait(uint64_t signalValue) const
{
    mCommandQueueData.Wait(signalValue);
}


//-------------------------------------------------------------------------------------------------
void yaget::render::platform::CommandQueues::CQ::Execute(ID3D12GraphicsCommandList4* commandList)
{
    std::vector<ID3D12GraphicsCommandList4*> commands{ commandList };

    Execute(commands);
}


//-------------------------------------------------------------------------------------------------
void yaget::render::platform::CommandQueues::CQ::Execute(std::vector<ID3D12GraphicsCommandList4*> commands)
{
    mCommandQueueData.mCommandQueue->ExecuteCommandLists(static_cast<UINT>(commands.size()), (ID3D12CommandList* const*)&commands[0]);
}


//-------------------------------------------------------------------------------------------------
ID3D12CommandQueue* yaget::render::platform::CommandQueues::CQ::GetCommandQueue() const
{
    return mCommandQueueData.mCommandQueue.Get();
}
