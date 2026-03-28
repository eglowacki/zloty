#include "Core/ErrorHandlers.h"
#include "magic_enum/magic_enum.hpp"
#include "Render/Commands/RenderAllocator.h"
#include "Render/Commands/RenderCommandList.h"
#include "Render/EnumConversion.h"
#include "Render/Platform/DeviceDebugger.h"

#include <d3dx12.h>

namespace
{
    yaget::render::ComPtr<ID3D12GraphicsCommandList> CreateCommandList(ID3D12Device* device, yaget::render::commands::Type type)
    {
        using namespace yaget;

        render::ComPtr<ID3D12Device4> device4;
        HRESULT hr = device->QueryInterface(IID_PPV_ARGS(&device4));
        error_handlers::ThrowOnError(hr, "Could not get ID3D12Device4 interface from ID3D12Device");

        auto commandType = render::ConvertCommandQueueType(type);
        render::ComPtr<ID3D12GraphicsCommandList4> commandList;

        hr = device4->CreateCommandList1(0, commandType, D3D12_COMMAND_LIST_FLAG_NONE, IID_PPV_ARGS(&commandList));
        error_handlers::ThrowOnError(hr, "Could not get create ID3D12GraphicsCommandList1 from ID3D12Device4");
        YAGET_RENDER_SET_DEBUG_NAME(commandList.Get(), std::format("CommandList-{}", magic_enum::enum_name(type)));

        return commandList;
    }

}


//-------------------------------------------------------------------------------------------------
yaget::render::commands::CommandList::CommandList(ID3D12Device* device, Type commandType)
    : mCommandType{ commandType }
    , mCommandList{ CreateCommandList(device, mCommandType) }
{
}


//-------------------------------------------------------------------------------------------------
yaget::render::commands::CommandList::~CommandList() = default;


//-------------------------------------------------------------------------------------------------
ID3D12GraphicsCommandList* yaget::render::commands::CommandList::GetDeviceCommandList() const
{
    YAGET_ASSERT(mCommandList, "Device Command List is nullptr.");

    return mCommandList.Get();
}


//-------------------------------------------------------------------------------------------------
void yaget::render::commands::CommandList::Close()
{
    HRESULT hr = GetDeviceCommandList()->Close();
    error_handlers::ThrowOnError(hr, "Could not close commandList");
}


//-------------------------------------------------------------------------------------------------
void yaget::render::commands::CommandList::Reset(Allocator* allocator)
{
    GetDeviceCommandList()->Reset(allocator->GetDeviceCommandAllocator(), nullptr);
}


//-------------------------------------------------------------------------------------------------
yaget::render::commands::CommandListStorage::CommandListStorage(ID3D12Device* device, uint32_t numBuffers)
{
    for (auto i = 0u; i < numBuffers; ++i)
    {
        constexpr uint32_t numCommandsPerBuffer = 6;

        auto& allocatorListDirect = mCommandLists[Type::Direct];
        auto& allocatorListCompute= mCommandLists[Type::Compute];
        auto& allocatorListCopy= mCommandLists[Type::Copy];

        for (auto numCommands = 0u; numCommands < numCommandsPerBuffer; ++numCommands)
        {
            allocatorListDirect.push_back({ std::make_unique<CommandList>(device, Type::Direct), false, i });
            allocatorListCompute.push_back({ std::make_unique<CommandList>(device, Type::Compute), false, i });
            allocatorListCopy.push_back({ std::make_unique<CommandList>(device, Type::Copy), false, i });
        }
    }
}


//-------------------------------------------------------------------------------------------------
yaget::render::commands::CommandListStorage::~CommandListStorage() = default;


//-------------------------------------------------------------------------------------------------
yaget::render::commands::CommandListStorage::CommandListHandle::CommandListHandle()
    : mCommandList{ nullptr }
    , mStorage{ nullptr }
{
}


//-------------------------------------------------------------------------------------------------
yaget::render::commands::CommandListStorage::CommandListHandle::CommandListHandle(CommandList* commandList, CommandListStorage* storage) 
    : mCommandList{ commandList }
    , mStorage{ storage }
{
}


//-------------------------------------------------------------------------------------------------
yaget::render::commands::CommandListStorage::CommandListHandle::~CommandListHandle()
{
    if (mStorage && mCommandList)
    {
        mStorage->FreeCommandList(mCommandList);
    }
    else
    {
        int z = 0;
        z;
    }
}


//-------------------------------------------------------------------------------------------------
yaget::render::commands::CommandListStorage::CommandListHandle::CommandListHandle(CommandListHandle&& other) noexcept
    : mCommandList{ std::exchange(other.mCommandList, nullptr) }
    , mStorage{ std::exchange(other.mStorage, nullptr) }
{
}


//-------------------------------------------------------------------------------------------------
yaget::render::commands::CommandListStorage::CommandListHandle& yaget::render::commands::CommandListStorage::CommandListHandle::operator=(CommandListHandle&& other) noexcept
{
    if (this != &other)
    {
        mCommandList = std::exchange(other.mCommandList, nullptr);
        mStorage = std::exchange(other.mStorage, nullptr);
    }

    return *this;
}


//-------------------------------------------------------------------------------------------------
yaget::render::commands::CommandListStorage::CommandListHandle yaget::render::commands::CommandListStorage::GetCommandList(Type commandType, uint32_t frameIndex)
{
    YAGET_ASSERT(mCommandLists.contains(commandType), std::format("Invalid CommandList list of type: '{}' for index: '{}'.", magic_enum::enum_name(commandType), frameIndex).c_str());

    auto& commandListHandles = mCommandLists[commandType];
    auto it = std::ranges::find_if(commandListHandles, [frameIndex](auto& element)
    {
        return element.mBufferIndex == frameIndex && element.mUsed == false;
    });

    YAGET_ASSERT(it != commandListHandles.end(), std::format("No available CommandList of type: '{}' for index: '{}'.", magic_enum::enum_name(commandType), frameIndex).c_str());
    it->mUsed = true;

    return { it->mCommandList.get(), this };
}


//-------------------------------------------------------------------------------------------------
void yaget::render::commands::CommandListStorage::FreeCommandList(CommandList* commandList)
{
    auto commandType = commandList->GetType();
    YAGET_ASSERT(mCommandLists.contains(commandType), std::format("Invalid CommandList list of type: '{}'.", magic_enum::enum_name(commandType)).c_str());

    auto& commandListHandles = mCommandLists[commandType];
    auto it = std::ranges::find_if(commandListHandles, [commandList, commandType](auto& element)
    {
        if (element.mCommandList.get() == commandList)
        {
            YAGET_ASSERT(element.mUsed == true, std::format("Used CommandList of type: {} is not marked as Used.", magic_enum::enum_name(commandType)).c_str());
            return true;
        }

        return false;
    });

    YAGET_ASSERT(it != commandListHandles.end(), std::format("No valid CommandList of type: {}.", magic_enum::enum_name(commandType)).c_str());
    it->mUsed = false;
}

