#include "App/AppUtilities.h"
#include "Metrics/Concurrency.h"
#include "Render/Commands/RenderAllocator.h"
#include "Render/Commands/RenderCommandListStates.h"
#include "Render/Commands/RenderQueue.h"
#include "Render/Device.h"
#include "Render/Platform/Adapter.h"
#include "Render/Platform/SwapChain.h"
#include "StringHelpers.h"
#include "Time/GameClock.h"

#include <d3dx12.h>
#include <ranges>


//-------------------------------------------------------------------------------------------------
yaget::render::DeviceB::DeviceB(app::WindowFrame windowFrame, const yaget::render::info::Adapter& adapterInfo)
    : mWindowFrame{ windowFrame }
    , mNumBackBuffers{ mWindowFrame.GetSurface().NumBackBuffers() }
    , mAdapter{ std::make_unique<platform::Adapter>(mWindowFrame, adapterInfo) }
    , mQueueStorage{ std::make_unique<commands::QueueStorage>(mAdapter->GetDevice()) }
    , mAllocatorStorage{ std::make_unique<commands::AllocatorStorage>(mAdapter->GetDevice(), mNumBackBuffers) }
    , mCommandListStorage{ std::make_unique<commands::CommandListStorage>(mAdapter->GetDevice(), mNumBackBuffers) }
    , mSwapChain{ std::make_unique<platform::SwapChain>(mWindowFrame, adapterInfo, mAdapter->GetDevice(), mAdapter->GetFactory(), mQueueStorage->GetQueue(commands::Type::Direct)->GetDeviceCommandQueue()) }
{
    for (uint32_t i = 0; i < static_cast<uint32_t>(mNumBackBuffers); ++i)
    {
        mFrameFenceValues[i] = 0;
    }

    YLOG_INFO("DEVI", "Device created and initialized.");
    //PIXSetMarker(0x0, "Device created.");
}


//-------------------------------------------------------------------------------------------------
yaget::render::DeviceB::~DeviceB()
{
    YLOG_INFO("DEVI", "Device shutdown.");
    Shutdown();
}


//-------------------------------------------------------------------------------------------------
void yaget::render::DeviceB::Resize()
{
    Waiter::Lock scoper(mWaiter);

    mQueueStorage->WaitForAllIdle();
    mSwapChain->Resize();
}


//-------------------------------------------------------------------------------------------------
void yaget::render::DeviceB::SurfaceStateChange()
{
    Resize();
}


//-------------------------------------------------------------------------------------------------
int64_t yaget::render::DeviceB::OnHandleRawInput(app::DisplaySurface::PlatformWindowHandle /*hWnd*/, uint32_t /*message*/, uint64_t /*wParam*/, int64_t /*lParam*/)
{
    return 0;
}


//-------------------------------------------------------------------------------------------------
void yaget::render::DeviceB::Shutdown()
{
    mQueueStorage->WaitForAllIdle();
}


//-------------------------------------------------------------------------------------------------
yaget::render::DeviceB::FrameCommands::FrameCommands(DeviceB& device, const time::GameClock& gameClock, metrics::Channel& channel)
    : mDevice{ &device}
    , mGameClock{ &gameClock }
    , mChannel{ &channel }
{
}


//-------------------------------------------------------------------------------------------------
yaget::render::DeviceB::FrameCommands::~FrameCommands()
{
    auto frameIndex = mDevice->mSwapChain->GetCurrentBackBufferIndex();

    mDevice->mSwapChain->Present(*mGameClock, *mChannel);

    auto queue = GetQueueStorage().GetQueue(commands::Type::Direct);
    auto fenceValue = queue->Signal();
    mDevice->mFrameFenceValues[frameIndex] = fenceValue;
    mDevice->mWaiter.Wait();
}


//-------------------------------------------------------------------------------------------------
yaget::render::commands::CommandList* yaget::render::DeviceB::FrameCommands::BeginFrame(const colors::Color* color)
{
    auto deviceRenderTarget = mDevice->mSwapChain->GetCurrentRenderTarget();
    auto frameIndex = mDevice->mSwapChain->GetCurrentBackBufferIndex();

    auto thisFrameFenceValue = mDevice->mFrameFenceValues[frameIndex];
    auto queue = GetQueueStorage().GetQueue(commands::Type::Direct);
    queue->WaitForFenceCPUBlocking(thisFrameFenceValue);

    auto allocator = GetAllocatorStorage().GetAllocator(commands::Type::Direct, frameIndex);
    auto deviceDescriptorHeap = mDevice->mSwapChain->GetDescriptorHeap();

    auto commandList = GetAvailableCommandList(commands::Type::Direct);
    allocator->Reset();
    commandList->Reset(allocator);

    commands::TransitionToRenderTarget(commandList, deviceRenderTarget, deviceDescriptorHeap, frameIndex);

    if (color)
    {
        commands::ClearRenderTarget(commandList, *color, deviceRenderTarget, deviceDescriptorHeap, frameIndex);
    }

    return commandList;
}


//-------------------------------------------------------------------------------------------------
void yaget::render::DeviceB::FrameCommands::EndFrame()
{
    auto deviceRenderTarget = mDevice->mSwapChain->GetCurrentRenderTarget();

    auto& lastCommandList = mCommandsToRender.back();
    commands::TransitionToPresent(lastCommandList, deviceRenderTarget);

    commands::Queue::CommandLists commandsList = mCommandsToRender | std::ranges::views::transform([this](const auto& command)
    {
        return command.Get();
    }) | std::ranges::to<std::vector>();

    auto queue = GetQueueStorage().GetQueue(commands::Type::Direct);
    queue->ExecuteCommandLists(commandsList);
}


//-------------------------------------------------------------------------------------------------
yaget::render::commands::CommandList* yaget::render::DeviceB::FrameCommands::GetAvailableCommandList(commands::Type commandType)
{
    auto commandListHandle = FindNextFreeCommandList(commandType);
    mCommandsToRender.emplace_back(std::move(commandListHandle));
    return mCommandsToRender.back().Get();
}


//-------------------------------------------------------------------------------------------------
yaget::render::commands::QueueStorage& yaget::render::DeviceB::FrameCommands::GetQueueStorage() const
{
    return *mDevice->mQueueStorage;
}


//-------------------------------------------------------------------------------------------------
yaget::render::commands::AllocatorStorage& yaget::render::DeviceB::FrameCommands::GetAllocatorStorage() const
{
    return *mDevice->mAllocatorStorage;
}


//-------------------------------------------------------------------------------------------------
yaget::render::commands::CommandListStorage& yaget::render::DeviceB::FrameCommands::GetCommandListStorage() const
{
    return *mDevice->mCommandListStorage;
}


//-------------------------------------------------------------------------------------------------
yaget::render::commands::CommandListStorage::CommandListHandle yaget::render::DeviceB::FrameCommands::FindNextFreeCommandList(commands::Type commandType)
{
    auto frameIndex = mDevice->mSwapChain->GetCurrentBackBufferIndex();

    auto commandList = GetCommandListStorage().GetCommandList(commandType, frameIndex);
    return commandList;
}


//-------------------------------------------------------------------------------------------------
yaget::render::DeviceB::FrameCommands yaget::render::DeviceB::GetFrameCommands(const time::GameClock& gameClock, metrics::Channel& channel)
{
    return FrameCommands{ *this, gameClock, channel };
}
