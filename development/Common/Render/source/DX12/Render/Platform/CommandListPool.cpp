#include "Render/Platform/CommandListPool.h"
#include "Render/Platform/DeviceDebugger.h"
#include "Render/RenderStringHelpers.h"
#include "App/AppUtilities.h"

#include <ranges>
#include <d3dx12.h>
//#include <dxgi1_6.h>


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

    yaget::render::platform::CommandListPool::CommandsList CreateCommandsList(ID3D12Device* device, D3D12_COMMAND_LIST_TYPE type, uint32_t numCommands)
    {
        using namespace yaget::render::platform;

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
    mFreeCommandsList[CommandQueue::Type::Direct] = CreateCommandsList(device, D3D12_COMMAND_LIST_TYPE_DIRECT, numCommands);
    mFreeCommandsList[CommandQueue::Type::Compute] = CreateCommandsList(device, D3D12_COMMAND_LIST_TYPE_COMPUTE, numCommands);
    mFreeCommandsList[CommandQueue::Type::Copy] = CreateCommandsList(device, D3D12_COMMAND_LIST_TYPE_COPY, numCommands);
}


//-------------------------------------------------------------------------------------------------
yaget::render::platform::CommandListPool::~CommandListPool() = default;


//-------------------------------------------------------------------------------------------------
yaget::render::platform::CommandListPool::Handle yaget::render::platform::CommandListPool::GetCommandList(yaget::render::platform::CommandQueue::Type type, ID3D12CommandAllocator* commandAllocator)
{
    YAGET_UTIL_THROW_ASSERT("DEVI", !mFreeCommandsList[type].empty(), fmt::format("There is no command list in free container for type: {}", yaget::conv::Convertor<yaget::render::platform::CommandQueue::Type>::ToString(type)));
    YAGET_UTIL_THROW_ASSERT("DEVI", commandAllocator, fmt::format("Command Allocoator parameter is NULL for type: {}", yaget::conv::Convertor<yaget::render::platform::CommandQueue::Type>::ToString(type)));

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
