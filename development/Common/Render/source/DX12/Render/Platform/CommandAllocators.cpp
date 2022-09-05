#include "Render/Platform/CommandAllocators.h"
#include "Render/Platform/DeviceDebugger.h"
#include "Render/RenderStringHelpers.h"
#include "App/AppUtilities.h"

#include <d3dx12.h>


namespace
{
    yaget::render::platform::CommandAllocators::AllocatorsList CreateCommandAllocator(ID3D12Device* device, D3D12_COMMAND_LIST_TYPE type, uint32_t numAllocators)
    {
        yaget::render::platform::CommandAllocators::AllocatorsList allocatorsList;

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
    mCommandAllocatorList[CommandQueue::Type::Direct] = CreateCommandAllocator(device, D3D12_COMMAND_LIST_TYPE_DIRECT, numAllocators);
    mCommandAllocatorList[CommandQueue::Type::Compute] = CreateCommandAllocator(device, D3D12_COMMAND_LIST_TYPE_COMPUTE, numAllocators);
    mCommandAllocatorList[CommandQueue::Type::Copy] = CreateCommandAllocator(device, D3D12_COMMAND_LIST_TYPE_COPY, numAllocators);

}


//-------------------------------------------------------------------------------------------------
yaget::render::platform::CommandAllocators::~CommandAllocators() = default;


//-------------------------------------------------------------------------------------------------
ID3D12CommandAllocator* yaget::render::platform::CommandAllocators::GetCommandAllocator(CommandQueue::Type type, uint32_t allocatorIndex) const
{
    YAGET_UTIL_THROW_ASSERT("DEVI", mCommandAllocatorList.find(type) != mCommandAllocatorList.end() && 
                                    mCommandAllocatorList.find(type)->second.size() > allocatorIndex, 
                                    fmt::format("There is no command allocator for type: {}", yaget::conv::Convertor<CommandQueue::Type>::ToString(type)));

    return mCommandAllocatorList.find(type)->second[allocatorIndex].Get();
}
