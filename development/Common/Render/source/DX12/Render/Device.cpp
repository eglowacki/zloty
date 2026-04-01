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

#include "magic_enum/magic_enum.hpp"

namespace
{
    yaget::render::commands::Queue::CommandLists GetCommandsList(const std::vector<yaget::render::commands::CommandListStorage::CommandListHandle>& commandsToRender, yaget::render::commands::Type commandType)
    {
        yaget::render::commands::Queue::CommandLists commandsList = commandsToRender |
            std::ranges::views::transform([](const auto& command)
        {
            return command.Get();
        }) |
            std::views::filter([commandType](const auto& command)
        {
            return command->GetType() == commandType;
        }) |
            std::ranges::to<std::vector>();

        return commandsList;
    }
    
}

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
        mFrameFenceValues[i];
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
yaget::render::platform::SwapChain& yaget::render::DeviceB::GetSwapChain() const
{
    return *mSwapChain.get();
}


//-------------------------------------------------------------------------------------------------
yaget::render::DeviceB::FrameCommands::FrameCommands(DeviceB& device, const time::GameClock& gameClock, metrics::Channel& channel)
    : FrameCommands(device, &gameClock, &channel, FrameType::Render)
{
}


//-------------------------------------------------------------------------------------------------
yaget::render::DeviceB::FrameCommands::FrameCommands(DeviceB& device)
    : FrameCommands(device, nullptr, nullptr, FrameType::Copy)
{
}


//-------------------------------------------------------------------------------------------------
yaget::render::DeviceB::FrameCommands::FrameCommands(DeviceB& device, const time::GameClock* gameClock, metrics::Channel* channel, FrameType frameType)
    : mDevice{ &device }
    , mFrameIndex{ mDevice->mSwapChain->GetCurrentBackBufferIndex() }
    , mGameClock{ gameClock }
    , mChannel{ channel }
    , mFrameType{ frameType }
{
    auto frameFenceValue = mDevice->GetFrameFenceValue(mFrameIndex, commands::Type::Direct);
    GetQueueStorage().GetQueue(commands::Type::Direct)->WaitForFenceCPUBlocking(frameFenceValue);

    frameFenceValue = mDevice->GetFrameFenceValue(mFrameIndex, commands::Type::Compute);
    GetQueueStorage().GetQueue(commands::Type::Compute)->WaitForFenceCPUBlocking(frameFenceValue);

    frameFenceValue = mDevice->GetFrameFenceValue(mFrameIndex, commands::Type::Copy);
    GetQueueStorage().GetQueue(commands::Type::Copy)->WaitForFenceCPUBlocking(frameFenceValue);

    GetAllocatorStorage().GetAllocator(commands::Type::Direct, mFrameIndex)->Reset();
    GetAllocatorStorage().GetAllocator(commands::Type::Compute, mFrameIndex)->Reset();
    GetAllocatorStorage().GetAllocator(commands::Type::Copy, mFrameIndex)->Reset();
}


//-------------------------------------------------------------------------------------------------
yaget::render::DeviceB::FrameCommands::~FrameCommands()
{
    if (mFrameType == FrameType::Render)
    {
        mDevice->mSwapChain->Present(*mGameClock, *mChannel);
    }

    for (const auto& commandHandle: mCommandsToRender)
    {
        auto type = commandHandle.Get()->GetType();

        auto queue = GetQueueStorage().GetQueue(type);
        auto fenceValue = queue->Signal();
        mDevice->SetFrameFenceValue(fenceValue, mFrameIndex, type);
    }

    if (mFrameType == FrameType::Render)
    {
        mDevice->mWaiter.Wait();
    }
    else if (mFrameType == FrameType::Copy)
    {
        auto frameFenceValue = mDevice->GetFrameFenceValue(mFrameIndex, commands::Type::Copy);
        GetQueueStorage().GetQueue(commands::Type::Copy)->WaitForFenceCPUBlocking(frameFenceValue);
    }
}


//-------------------------------------------------------------------------------------------------
yaget::render::commands::CommandList* yaget::render::DeviceB::FrameCommands::BeginFrame(const colors::Color* color)
{
    commands::Type currentType = commands::Type::Direct;
    if (mFrameType == FrameType::Render)
    {
        currentType = commands::Type::Direct;
    }
    else if (mFrameType == FrameType::Copy)
    {
        currentType = commands::Type::Copy;
    }
    else
    {
        YAGET_ASSERT(false, "Unhandled FrameType: '{}'.", magic_enum::enum_name(mFrameType));
    }

    auto commandList = GetAvailableCommandList(currentType);

    if (mFrameType == FrameType::Render)
    {
        auto deviceRenderTarget = mDevice->mSwapChain->GetCurrentRenderTarget();
        auto deviceDescriptorHeap = mDevice->mSwapChain->GetRTVDescriptorHeap();

        commands::TransitionToRenderTarget(commandList, deviceRenderTarget, deviceDescriptorHeap, mFrameIndex);

        if (color)
        {
            commands::ClearRenderTarget(commandList, *color, deviceRenderTarget, deviceDescriptorHeap, mFrameIndex);
        }
    }

    return commandList;
}


//-------------------------------------------------------------------------------------------------
void yaget::render::DeviceB::FrameCommands::EndFrame()
{
    if (mFrameType == FrameType::Render)
    {
        auto currentCommandType = commands::Type::Direct;
        if (auto commandsList = GetCommandsList(mCommandsToRender, currentCommandType); !commandsList.empty())
        {
            auto deviceRenderTarget = mDevice->mSwapChain->GetCurrentRenderTarget();

            auto& lastCommandList = commandsList.back();
            commands::TransitionToPresent(lastCommandList, deviceRenderTarget);
        }
    }

    auto currentCommandType = commands::Type::Direct;
    auto commandsList = GetCommandsList(mCommandsToRender, currentCommandType);
    GetQueueStorage().GetQueue(currentCommandType)->ExecuteCommandLists(commandsList);

    currentCommandType = commands::Type::Compute;
    commandsList = GetCommandsList(mCommandsToRender, currentCommandType);
    GetQueueStorage().GetQueue(currentCommandType)->ExecuteCommandLists(commandsList);

    currentCommandType = commands::Type::Copy;
    commandsList = GetCommandsList(mCommandsToRender, currentCommandType);
    GetQueueStorage().GetQueue(currentCommandType)->ExecuteCommandLists(commandsList);
}


//-------------------------------------------------------------------------------------------------
yaget::render::commands::CommandList* yaget::render::DeviceB::FrameCommands::GetAvailableCommandList(commands::Type commandType)
{
    auto commandListHandle = FindNextFreeCommandList(commandType);
    mCommandsToRender.emplace_back(std::move(commandListHandle));
    auto commandList = mCommandsToRender.back().Get();

    auto allocator = GetAllocatorStorage().GetAllocator(commandType, mFrameIndex);
    commandList->Reset(allocator);

    return commandList;
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
    auto commandList = GetCommandListStorage().GetCommandList(commandType, mFrameIndex);
    return commandList;
}


//-------------------------------------------------------------------------------------------------
yaget::render::DeviceB::FrameCommands yaget::render::DeviceB::GetFrameCommands(const time::GameClock& gameClock, metrics::Channel& channel)
{
    return FrameCommands{ *this, gameClock, channel };
}


//-------------------------------------------------------------------------------------------------
yaget::render::DeviceB::FrameCommands yaget::render::DeviceB::GetCopyCommands()
{
    return FrameCommands{ *this };
}


//-------------------------------------------------------------------------------------------------
yaget::render::DeviceB::MemoryTrackerReporter::~MemoryTrackerReporter()
{
    yaget::render::platform::PrintD3D12MAMemoryTracker();
}


//-------------------------------------------------------------------------------------------------
void yaget::render::DeviceB::SetFrameFenceValue(uint64_t fenceValue, uint32_t frameIndex, commands::Type type)
{
    mFrameFenceValues[frameIndex][static_cast<uint32_t>(type)] = fenceValue;
}


//-------------------------------------------------------------------------------------------------
uint64_t yaget::render::DeviceB::GetFrameFenceValue(uint32_t frameIndex, commands::Type type)
{
    auto frameValue = mFrameFenceValues[frameIndex][static_cast<uint32_t>(type)];
    return frameValue;
}
