#include "Render/Platform/CommandAllocators.h"
#include "Render/Platform/DeviceDebugger.h"
#include "Render/RenderStringHelpers.h"
#include "Render/EnumConversion.h"
#include "App/AppUtilities.h"

#include <d3dx12.h>


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
            HRESULT hr = device->CreateCommandAllocator(type, IID_PPV_ARGS(&commandAllocator));
            YAGET_UTIL_THROW_ON_RROR(hr, "Could not create DX12 Command Allocator");

            YAGET_RENDER_SET_DEBUG_NAME(commandAllocator.Get(), "Yaget Command Allocator");

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
    HRESULT hr = allocator->Reset();
    YAGET_UTIL_THROW_ON_RROR(hr, "Could not reset allocator");

    return allocator;
}
