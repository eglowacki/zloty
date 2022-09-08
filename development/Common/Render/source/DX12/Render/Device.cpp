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

//-------------------------------------------------------------------------------------------------
yaget::render::DeviceB::DeviceB(app::WindowFrame windowFrame, const yaget::render::info::Adapter& adapterInfo)
    : mWindowFrame{ windowFrame }
    , mAdapter{ std::make_unique<platform::Adapter>(mWindowFrame, adapterInfo) }
    , mSwapChain{ std::make_unique<platform::SwapChain>(mWindowFrame, adapterInfo, mAdapter->GetDevice(), mAdapter->GetFactory()) }
    , mCommandAllocators{ std::make_unique<platform::CommandAllocators>(mAdapter->GetDevice(), mWindowFrame.GetSurface().NumBackBuffers()) }
    , mCommandQueues{ std::make_unique<platform::CommandQueues>(mAdapter->GetDevice()) }
    , mCommandListPool{ std::make_unique<platform::CommandListPool>(mAdapter->GetDevice(), NumCommands) }
    , mPolygon{ std::make_unique<Polygon>(mAdapter->GetDevice(), mAdapter->GetAllocator(), false /*useTwo*/) }
    , mPolygon2{ std::make_unique<Polygon>(mAdapter->GetDevice(), mAdapter->GetAllocator(), true /*useTwo*/) }
{

    YLOG_INFO("DEVI", "Device created and initialized.");
    PIXSetMarker(0x0, "Device created.");
}


//-------------------------------------------------------------------------------------------------
yaget::render::DeviceB::~DeviceB()
{
    YLOG_INFO("DEVI", "Device shutdown.");
}


//-------------------------------------------------------------------------------------------------
void yaget::render::DeviceB::Resize()
{
    Waiter::Lock scoper(mWaiter);

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
    auto allocator = mCommandAllocators->GetCommandAllocator(platform::CommandQueue::Type::Direct, mSwapChain->GetCurrentBackBufferIndex());
    auto handle = mCommandListPool->GetCommandList(platform::CommandQueue::Type::Direct, allocator);

    auto commandQueue = mCommandQueues->GetCommandQueue(platform::CommandQueue::Type::Direct);
    commandQueue;

    //ID3D12CommandList* commands[] = { handle.GetCommandList() };
    //commandQueue->ExecuteCommandLists(1, commands);

    const std::vector<Polygon*> polygons{ mPolygon.get(), mPolygon2.get() };
    mSwapChain->Render(polygons, gameClock, channel);

    //constexpr time::TimeUnits_t waitTime = 1;
    //constexpr time::TimeUnits_t unitType = time::kMilisecondUnit;
    //yaget::platform::Sleep(waitTime, unitType);

    HRESULT hr = handle.GetCommandList()->Close();
    YAGET_UTIL_THROW_ON_RROR(hr, "Could not close command list for polygon");

    mWaiter.Wait();
}
