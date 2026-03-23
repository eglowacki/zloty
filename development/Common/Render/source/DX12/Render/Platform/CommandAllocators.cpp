#include "Render/Platform/CommandAllocators.h"
#include "Render/Platform/DeviceDebugger.h"
#include "Render/RenderStringHelpers.h"
#include "Render/EnumConversion.h"
#include "App/AppUtilities.h"

#include <d3dx12.h>

#include "Core/ErrorHandlers.h"

#include "magic_enum/magic_enum.hpp"


namespace
{
    yaget::render::platform::CommandAllocators::AllocatorsList CreateCommandAllocator(ID3D12Device* device, yaget::render::platform::CommandQueue::Type cqType, uint32_t numAllocators)
    {
        using namespace yaget::render::platform;

       CommandAllocators::AllocatorsList allocatorsList;

        D3D12_COMMAND_LIST_TYPE type = yaget::render::ConvertCommandQueueType(cqType);

        for (auto i = 0u; i < numAllocators; ++i)
        {
            Microsoft::WRL::ComPtr<ID3D12CommandAllocator> commandAllocator;
            const HRESULT hr = device->CreateCommandAllocator(type, IID_PPV_ARGS(&commandAllocator));
            yaget::error_handlers::ThrowOnError(hr, "Could not create DX12 Command Allocator");

            const auto debugName = std::format("Command Allocator-T_{}-I_{}", magic_enum::enum_name(cqType), i);
            YAGET_RENDER_SET_DEBUG_NAME(commandAllocator.Get(), debugName);

            allocatorsList.push_back(commandAllocator);
        }

        return allocatorsList;
    }

} // namespace


//-------------------------------------------------------------------------------------------------
yaget::render::platform::CommandAllocators::CommandAllocators(ID3D12Device* device, uint32_t numAllocators)
{
    using CQIterator = yaget::meta::EnumIterator<CommandQueue::Type, CommandQueue::Type::Direct, CommandQueue::Type::End, false>;

    for (CommandQueue::Type i : CQIterator()) 
    {
        mCommandAllocatorList[i] = CreateCommandAllocator(device, i, numAllocators);
    }
}


//-------------------------------------------------------------------------------------------------
yaget::render::platform::CommandAllocators::~CommandAllocators() = default;


//-------------------------------------------------------------------------------------------------
ID3D12CommandAllocator* yaget::render::platform::CommandAllocators::GetCommandAllocator(CommandQueue::Type type, uint32_t allocatorIndex) const
{
    YAGET_ASSERT(mCommandAllocatorList.find(type) != mCommandAllocatorList.end() && 
                                    mCommandAllocatorList.find(type)->second.size() > allocatorIndex, 
                                    "There is no command allocator for type: %s", yaget::conv::Convertor<CommandQueue::Type>::ToString(type).c_str());

    auto allocator = mCommandAllocatorList.find(type)->second[allocatorIndex].Get();
    const HRESULT hr = allocator->Reset();
    YLOG_WARNING("REND", "================== Resetting Command Allocator. Type: '%d', Index: '%d'", type,allocatorIndex);
    error_handlers::ThrowOnError(hr, "Could not reset allocator");

    return allocator;
}
