#include "Render/Platform/CommandListPool.h"
#include "Render/Platform/DeviceDebugger.h"
#include "Render/RenderStringHelpers.h"
#include "Render/EnumConversion.h"
#include "App/AppUtilities.h"

#include <ranges>
#include <d3dx12.h>


namespace
{
    //-------------------------------------------------------------------------------------------------
    Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList4> CreateCommandList(ID3D12Device* device, D3D12_COMMAND_LIST_TYPE type)
    {
        Microsoft::WRL::ComPtr<ID3D12Device4> device4;
        HRESULT hr = device->QueryInterface<ID3D12Device4>(&device4);
        YAGET_UTIL_THROW_ON_RROR(hr, "Could not get ID3D12Device4 interface");

        Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList1> commandList;
        hr = device4->CreateCommandList1(0, type, D3D12_COMMAND_LIST_FLAG_NONE, IID_PPV_ARGS(&commandList));
        YAGET_UTIL_THROW_ON_RROR(hr, "Could not create DX12 Command List");

        Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList4> commandList4;
        hr = commandList.As(&commandList4);
        YAGET_UTIL_THROW_ON_RROR(hr, "Could not get ID3D12GraphicsCommandList4 interface");

        YAGET_RENDER_SET_DEBUG_NAME(commandList4.Get(), "Yaget Command List");

        return commandList4;
    }

    yaget::render::platform::CommandListPool::CommandsList CreateCommandsList(ID3D12Device* device, yaget::render::platform::CommandQueue::Type cqType, uint32_t numCommands)
    {
        using namespace yaget::render::platform;

        D3D12_COMMAND_LIST_TYPE type = yaget::render::ConvertCommandQueueType(cqType);

        CommandListPool::CommandsList commandsList;

        for (auto i = 0u; i < numCommands; ++i)
        {
            commandsList.push(CreateCommandList(device, type));
        }

        return commandsList;
    }

} // namespace


//-------------------------------------------------------------------------------------------------
yaget::render::platform::CommandListPool::CommandListPool(ID3D12Device* device, uint32_t numCommands)
{
    using CQIterator = yaget::meta::EnumIterator<CommandQueue::Type, CommandQueue::Type::Direct, CommandQueue::Type::End, false>;

    for (CommandQueue::Type i : CQIterator()) 
    {
        mFreeCommandsList[i] = CreateCommandsList(device, i, numCommands);
    }
}


//-------------------------------------------------------------------------------------------------
yaget::render::platform::CommandListPool::~CommandListPool() = default;


//-------------------------------------------------------------------------------------------------
yaget::render::platform::CommandListPool::Handle yaget::render::platform::CommandListPool::GetCommandList(yaget::render::platform::CommandQueue::Type type, ID3D12CommandAllocator* commandAllocator)
{
    YAGET_ASSERT(!mFreeCommandsList[type].empty(), "There is no command list in free container for type: %s", yaget::conv::Convertor<yaget::render::platform::CommandQueue::Type>::ToString(type).c_str());
    YAGET_ASSERT(commandAllocator, "Command Allocoator parameter is NULL for type: %s", yaget::conv::Convertor<yaget::render::platform::CommandQueue::Type>::ToString(type).c_str());

    auto command = mFreeCommandsList[type].front();
    mFreeCommandsList[type].pop();

    HRESULT hr = command->Reset(commandAllocator, nullptr);
    YAGET_UTIL_THROW_ON_RROR(hr, "Could not reset Command List");

    return Handle(*this, command.Get(), type);
}


//-------------------------------------------------------------------------------------------------
void yaget::render::platform::CommandListPool::FreeUsed(ComPtr<ID3D12GraphicsCommandList4> commandList, CommandQueue::Type type)
{
    mFreeCommandsList[type].push(commandList);
}


//-------------------------------------------------------------------------------------------------
yaget::render::platform::CommandListPool::Handle::Handle(CommandListPool& commandPool, ComPtr<ID3D12GraphicsCommandList4> commandList, CommandQueue::Type type)
    : mCommandPool(commandPool)
    , mCommandList(commandList)
    , mType(type)
{
    YAGET_UTIL_THROW_ASSERT("DEVI", mCommandList, "commandList paramter is NULL.");
}


//-------------------------------------------------------------------------------------------------
yaget::render::platform::CommandListPool::Handle::~Handle()
{
    mCommandPool.FreeUsed(mCommandList, mType);
}


//-------------------------------------------------------------------------------------------------
void yaget::render::platform::CommandListPool::Handle::TransitionToRenderTarget(ID3D12Resource* renderTarget, ID3D12DescriptorHeap* descriptorHeap, uint32_t frameIndex)
{
    YAGET_ASSERT(renderTarget, "Parameter renderTarget is null.");

    const CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(renderTarget, D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
    mCommandList->ResourceBarrier(1, &barrier);

    D3D12_RESOURCE_DESC desc = renderTarget->GetDesc();

    D3D12_VIEWPORT viewport = {};
    viewport.Width = static_cast<float>(desc.Width);
    viewport.Height = static_cast<float>(desc.Height);
    viewport.MinDepth = 0.0f;
    viewport.MaxDepth = 1.0f;
    mCommandList->RSSetViewports(1, &viewport);

    D3D12_RECT rect = {};
    rect.right = static_cast<LONG>(desc.Width);
    rect.bottom = static_cast<LONG>(desc.Height);
    mCommandList->RSSetScissorRects(1, &rect);

    ComPtr<ID3D12Device4> device;
    HRESULT hr = renderTarget->GetDevice(IID_PPV_ARGS(&device));
    YAGET_UTIL_THROW_ON_RROR(hr, "Could not get device from render target");

    const auto descriptorHandleSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

    const CD3DX12_CPU_DESCRIPTOR_HANDLE rtv(descriptorHeap->GetCPUDescriptorHandleForHeapStart(), frameIndex, descriptorHandleSize);
    mCommandList->OMSetRenderTargets(1, &rtv, false, nullptr);
}


//-------------------------------------------------------------------------------------------------
void yaget::render::platform::CommandListPool::Handle::TransitionToPresent(ID3D12Resource* renderTarget, bool closeCommand)
{
    YAGET_ASSERT(renderTarget, "Parameter renderTarget is null.");

    CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(renderTarget, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
    mCommandList->ResourceBarrier(1, &barrier);

    if (closeCommand)
    {
        HRESULT hr = mCommandList->Close();
        YAGET_UTIL_THROW_ON_RROR(hr, "Could not close command list");
    }
}


//-------------------------------------------------------------------------------------------------
void yaget::render::platform::CommandListPool::Handle::ClearRenderTarget(const colors::Color& color, ID3D12Resource* renderTarget, ID3D12DescriptorHeap* descriptorHeap, uint32_t frameIndex)
{
    YAGET_ASSERT(renderTarget, "Parameter renderTarget is null.");
    YAGET_ASSERT(descriptorHeap, "Parameter descriptorHeap is null.");

    const float clearColor[] = { color.R(), color.B(), color.G(), color.A() };

    ComPtr<ID3D12Device4> device;
    HRESULT hr = renderTarget->GetDevice(IID_PPV_ARGS(&device));
    YAGET_UTIL_THROW_ON_RROR(hr, "Could not get device from render target");

    const auto descriptorHandleSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

    const CD3DX12_CPU_DESCRIPTOR_HANDLE rtv(descriptorHeap->GetCPUDescriptorHandleForHeapStart(), frameIndex, descriptorHandleSize);
    mCommandList->ClearRenderTargetView(rtv, clearColor, 0, nullptr);
}
