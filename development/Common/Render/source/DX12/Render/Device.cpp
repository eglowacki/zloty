#include "App/AppUtilities.h"
#include "Metrics/Concurrency.h"
#include "Render/Device.h"
#include "Render/Metrics/RenderMetrics.h"
#include "Render/Platform/Adapter.h"
#include "Render/Platform/CommandAllocators.h"
#include "Render/Platform/CommandListPool.h"
#include "Render/Platform/CommandQueue.h"
#include "Render/Platform/SwapChain.h"
#include "StringHelpers.h"
#include "Time/GameClock.h"
#include <d3d12.h>


namespace
{
    using namespace yaget;

    constexpr uint32_t NumCommands = 6;

    struct Framer
    {
        Framer(const time::GameClock& gameClock, metrics::Channel& channel, render::DeviceB& device, const colors::Color* color)
            : mDevice(device)
            , mFrameIndex(mDevice.mSwapChain->GetCurrentBackBufferIndex())
            , mCommandQueue(Framer::GetCommandQueueHandle(device, mFrameIndex))
            , mCommandHandle(Framer::GetCommandHandle(device, mFrameIndex, color))
            , mGameClock(gameClock)
            , mChannel(channel)
        {}

        ~Framer()
        {
            mCommandHandle.TransitionToPresent(true /*closeCommand*/);
            mCommandQueue.Execute(mCommandHandle);
            mDevice.mFrameFenceValues[mFrameIndex] = mCommandQueue.Signal();

            mDevice.mSwapChain->Present(mGameClock, mChannel);

            mDevice.mWaiter.Wait();
        }

        ID3D12GraphicsCommandList* GetCommandList()
        {
            return mCommandHandle;
        }

        static render::platform::CommandQueues::CQ GetCommandQueueHandle(render::DeviceB& device, uint32_t frameIndex)
        {
            auto commandQueues = device.mCommandQueues->GetCQ(render::platform::CommandQueue::Type::Direct, false /*finished*/);
            const auto fenceValue = device.mFrameFenceValues[frameIndex];
            commandQueues.Wait(fenceValue);

            return std::move(commandQueues);
        }

        static render::platform::CommandListPool::Handle GetCommandHandle(render::DeviceB& device, uint32_t frameIndex, const colors::Color* color)
        {
            auto allocator = device.mCommandAllocators->GetCommandAllocator(render::platform::CommandQueue::Type::Direct, frameIndex);
            auto renderTarget = device.mSwapChain->GetCurrentRenderTarget();
            auto descriptorHeap = device.mSwapChain->GetDescriptorHeap();

            auto commandHandle = device.mCommandListPool->GetCommandList(render::platform::CommandQueue::Type::Direct, allocator, renderTarget, descriptorHeap, frameIndex);
            commandHandle.TransitionToRenderTarget();

            if (color)
            {
                commandHandle.ClearRenderTarget(*color);
            }

            return commandHandle;
        }

        render::DeviceB& mDevice;
        uint32_t mFrameIndex = 0;
        render::platform::CommandQueues::CQ mCommandQueue;
        render::platform::CommandListPool::Handle mCommandHandle;
        const time::GameClock& mGameClock;
        metrics::Channel& mChannel;
    };

}


//-------------------------------------------------------------------------------------------------
yaget::render::DeviceB::DeviceB(app::WindowFrame windowFrame, const yaget::render::info::Adapter& adapterInfo)
    : mWindowFrame{ windowFrame }
    , mAdapter{ std::make_unique<platform::Adapter>(mWindowFrame, adapterInfo) }
    , mCommandAllocators{ std::make_unique<platform::CommandAllocators>(mAdapter->GetDevice(), mWindowFrame.GetSurface().NumBackBuffers()) }
    , mCommandQueues{ std::make_unique<platform::CommandQueues>(mAdapter->GetDevice()) }
    , mSwapChain{ std::make_unique<platform::SwapChain>(mWindowFrame, adapterInfo, mAdapter->GetDevice(), mAdapter->GetFactory(), mCommandQueues->GetCQ(platform::CommandQueue::Type::Direct, false /*finished*/).GetCommandQueue()) }
    , mCommandListPool{ std::make_unique<platform::CommandListPool>(mAdapter->GetDevice(), NumCommands) }
{
    for (uint32_t i = 0; i < static_cast<uint32_t>(mWindowFrame.GetSurface().NumBackBuffers()); ++i)
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
void yaget::render::DeviceB::Shutdown()
{
    mCommandQueues->Reset();
}


//-------------------------------------------------------------------------------------------------
yaget::render::DeviceB::FramerHandle yaget::render::DeviceB::GetFramerHandle(const time::GameClock& gameClock, metrics::Channel& channel, const colors::Color* color)
{
    return { gameClock, channel, *this, color };
}


//-------------------------------------------------------------------------------------------------
yaget::render::DeviceB::FramerHandle::FramerHandle(const time::GameClock& gameClock, metrics::Channel& channel, DeviceB& device, const colors::Color* color)
    : mFramer(std::make_shared<Framer>(gameClock, channel, device, color))
{
}


ID3D12GraphicsCommandList* yaget::render::DeviceB::FramerHandle::GetCommandList()
{
    return mFramer->GetCommandList();
}
