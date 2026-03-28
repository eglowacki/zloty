#include <ranges>

#include "Render/Commands/RenderAllocator.h"
#include "Render/EnumConversion.h"
#include "Render/Platform/DeviceDebugger.h"


namespace
{
    //-------------------------------------------------------------------------------------------------
    yaget::render::ComPtr<ID3D12CommandAllocator> CreateCommandAllocator(ID3D12Device* device, yaget::render::commands::Type type)
    {
        using namespace yaget;

        render::ComPtr<ID3D12CommandAllocator> commandAllocator;
        auto commandType = render::ConvertCommandQueueType(type);

        const HRESULT hr = device->CreateCommandAllocator(commandType, IID_PPV_ARGS(&commandAllocator));
        yaget::error_handlers::ThrowOnError(hr, "Could not create DX12 Command Allocator");

        const auto debugName = std::format("Command Allocator-{}", magic_enum::enum_name(type));
        YAGET_RENDER_SET_DEBUG_NAME(commandAllocator.Get(), debugName);

        return commandAllocator;
    }

}


//-------------------------------------------------------------------------------------------------
yaget::render::commands::Allocator::Allocator(ID3D12Device* device, Type commandType)
    : mCommandAllocator{ CreateCommandAllocator(device, commandType) }
{
}


//-------------------------------------------------------------------------------------------------
yaget::render::commands::Allocator::~Allocator() = default;


//-------------------------------------------------------------------------------------------------
ID3D12CommandAllocator* yaget::render::commands::Allocator::GetDeviceCommandAllocator() const
{
    return mCommandAllocator.Get();
}


//-------------------------------------------------------------------------------------------------
void yaget::render::commands::Allocator::Reset()
{
    HRESULT hr = GetDeviceCommandAllocator()->Reset();
    error_handlers::ThrowOnError(hr, "Could not Reset DX12 Command Allocator");
}


//-------------------------------------------------------------------------------------------------
yaget::render::commands::AllocatorStorage::AllocatorStorage(ID3D12Device* device, uint32_t numBuffers)
{
    for (auto i = 0u; i < numBuffers; ++i)
    {
        auto& allocatorListDirect = mAllocators[Type::Direct];
        allocatorListDirect.push_back({ std::make_unique<Allocator>(device, Type::Direct), i });

        auto& allocatorListCompute= mAllocators[Type::Compute];
        allocatorListCompute.push_back({ std::make_unique<Allocator>(device, Type::Compute), i});

        auto& allocatorListCopy= mAllocators[Type::Copy];
        allocatorListCopy.push_back({ std::make_unique<Allocator>(device, Type::Copy), i });
    }
}


//-------------------------------------------------------------------------------------------------
yaget::render::commands::AllocatorStorage::~AllocatorStorage() = default;


//-------------------------------------------------------------------------------------------------
void yaget::render::commands::AllocatorStorage::Reset()
{
    for (auto& values : mAllocators | std::views::values)
    {
        for (auto& allocatorEntry : values)
        {
            allocatorEntry.mAllocator->Reset();
        }
    }
}


//-------------------------------------------------------------------------------------------------
yaget::render::commands::Allocator* yaget::render::commands::AllocatorStorage::GetAllocator(Type commandType, uint32_t frameIndex)
{
    YAGET_ASSERT(mAllocators.contains(commandType) && frameIndex < mAllocators.find(commandType)->second.size(), std::format("Invalid mAllocator list of type: '{}' for index: '{}'.",
        magic_enum::enum_name(commandType), frameIndex).c_str());

    auto& allocatorList = mAllocators[commandType];
    auto it = std::ranges::find_if(allocatorList, [frameIndex](auto& element)
    {
        return element.mBufferIndex == frameIndex;
    });

    YAGET_ASSERT(it != allocatorList.end(), std::format("No available Allocator of type: '{}' for index: '{}'.", magic_enum::enum_name(commandType), frameIndex).c_str());

    return it->mAllocator.get();
}


