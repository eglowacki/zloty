#include "Render/Device.h"
#include "App/AppUtilities.h"
#include "Debugging/DevConfiguration.h"
#include "Metrics/Concurrency.h"
#include "Platform/Adapter.h"
#include "Render/Metrics/RenderMetrics.h"
#include "Render/Platform/CommandAllocators.h"
#include "Render/Platform/CommandListPool.h"
#include "Render/Platform/SwapChain.h"
#include "Render/Polygons/Polygon.h"
#include "StringHelpers.h"
#include "Time/GameClock.h"

#include <d3d12.h>


namespace
{
    constexpr uint32_t NumCommands = 6;
}

yaget::render::ColorInterpolator::ColorInterpolator(const colors::Color& startColor, const colors::Color& endColor)
    : mStartColor{ startColor }
    , mEndColor{ endColor }
{
}


//-------------------------------------------------------------------------------------------------
colors::Color yaget::render::ColorInterpolator::GetColor(const time::GameClock& gameClock)
{
    const math3d::Color adjustedColor = math3d::Color::Lerp(mStartColor, mEndColor, mCurrentColorT);
    mCurrentColorT += (gameClock.GetDeltaTimeSecond() * mColorTDirection) * 0.75f;
    if (mCurrentColorT > 1.0f)
    {
        mColorTDirection = -1.0f;
    }
    else if (mCurrentColorT < 0.0f)
    {
        mColorTDirection = 1.0f;
    }

    return adjustedColor;
}


//-------------------------------------------------------------------------------------------------
yaget::render::DeviceB::DeviceB(app::WindowFrame windowFrame, const yaget::render::info::Adapter& adapterInfo)
    : mWindowFrame{ windowFrame }
    , mAdapter{ std::make_unique<platform::Adapter>(mWindowFrame, adapterInfo) }
    , mPolygon{ std::make_unique<Polygon>(mAdapter->GetDevice(), mAdapter->GetAllocator(), false /*useTwo*/) }
    , mPolygon2{ std::make_unique<Polygon>(mAdapter->GetDevice(), mAdapter->GetAllocator(), true /*useTwo*/) }
    , mCommandAllocators{ std::make_unique<platform::CommandAllocators>(mAdapter->GetDevice(), mWindowFrame.GetSurface().NumBackBuffers()) }
    , mCommandQueues{ std::make_unique<platform::CommandQueues>(mAdapter->GetDevice()) }
    , mSwapChain{ std::make_unique<platform::SwapChain>(mWindowFrame, adapterInfo, mAdapter->GetDevice(), mAdapter->GetFactory(), mCommandQueues->GetCQ(platform::CommandQueue::Type::Direct, false /*finished*/).GetCommandQueue()) }
    , mCommandListPool{ std::make_unique<platform::CommandListPool>(mAdapter->GetDevice(), NumCommands) }
    , mColorInterpolator({ 0.4f, 0.6f, 0.9f, 1.0f }, { 0.6f, 0.9f, 0.4f, 1.0f })
{
    for (uint32_t i = 0; i < static_cast<uint32_t>(mWindowFrame.GetSurface().NumBackBuffers()); ++i)
    {
        mFrameFenceValues[i] = 0;
    }

    YLOG_INFO("DEVI", "Device created and initialized.");
    PIXSetMarker(0x0, "Device created.");
}


//-------------------------------------------------------------------------------------------------
yaget::render::DeviceB::~DeviceB()
{
    YLOG_INFO("DEVI", "Device shutdown.");
    mCommandQueues->Reset();
}


//-------------------------------------------------------------------------------------------------
void yaget::render::DeviceB::Resize()
{
    Waiter::Lock scoper(mWaiter);

    mCommandQueues->Reset();
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
void yaget::render::DeviceB::RenderFrame(const time::GameClock& gameClock, metrics::Channel& channel)
{
    const auto frameIndex = mSwapChain->GetCurrentBackBufferIndex();

    const auto fenceValue = mFrameFenceValues[frameIndex];
    auto commandQueue = mCommandQueues->GetCQ(platform::CommandQueue::Type::Direct, false /*finished*/);
    commandQueue.Wait(fenceValue);

    auto allocator = mCommandAllocators->GetCommandAllocator(platform::CommandQueue::Type::Direct, frameIndex);

    auto renderTarget = mSwapChain->GetCurrentRenderTarget();
    auto descriptorHeap = mSwapChain->GetDescriptorHeap();

    const colors::Color color = mColorInterpolator.GetColor(gameClock);

    auto commandHandleA = mCommandListPool->GetCommandList(platform::CommandQueue::Type::Direct, allocator, renderTarget, descriptorHeap, frameIndex);
    commandHandleA.TransitionToRenderTarget();
    commandHandleA.ClearRenderTarget(color);

    mPolygon->Render(commandHandleA, {});

    commandHandleA.TransitionToPresent(true /*closeCommand*/);

    auto commandHandleB = mCommandListPool->GetCommandList(platform::CommandQueue::Type::Direct, allocator, renderTarget, descriptorHeap, frameIndex);
    commandHandleB.TransitionToRenderTarget();

    mPolygon2->Render(commandHandleB, {});

    commandHandleB.TransitionToPresent(true /*closeCommand*/);

    commandQueue.Execute({ commandHandleA, commandHandleB });
    mFrameFenceValues[frameIndex] = commandQueue.Signal();
    //commandQueue.Execute(commandHandleA);

    mSwapChain->Present(gameClock, channel);

    mWaiter.Wait();
}
