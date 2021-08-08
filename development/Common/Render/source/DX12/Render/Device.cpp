#include "Render/Device.h"
#include "App/AppUtilities.h"
#include "Metrics/Concurrency.h"
#include "Platform/Adapter.h"
#include "Render/Platform/SwapChain.h"
#include "StringHelpers.h"
#include "Time/GameClock.h"
#include "Debugging/DevConfiguration.h"


//-------------------------------------------------------------------------------------------------
yaget::render::DeviceB::DeviceB(app::WindowFrame windowFrame)
    : mWindowFrame{ windowFrame }
    , mAdapter{ std::make_unique<platform::Adapter>(mWindowFrame) }
    , mSwapChain{ std::make_unique<platform::SwapChain>(mWindowFrame, mAdapter->GetDevice(), mAdapter->GetFactory().Get()) }
{
    YLOG_INFO("DEVI", "Device created and initialized.");
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


YAGET_COMPILE_SUPRESS_START(4100, "unreferenced local variable")

//-------------------------------------------------------------------------------------------------
int64_t yaget::render::DeviceB::OnHandleRawInput(app::DisplaySurface::PlatformWindowHandle hWnd, uint32_t message, uint64_t wParam, int64_t lParam)
{
    return 0;
}


//-------------------------------------------------------------------------------------------------
void yaget::render::DeviceB::RenderFrame(const time::GameClock& gameClock, metrics::Channel& channel)
{
    mSwapChain->Render(gameClock, channel);

    //constexpr time::TimeUnits_t waitTime = 1;
    //constexpr time::TimeUnits_t unitType = time::kMilisecondUnit;
    //yaget::platform::Sleep(waitTime, unitType);

    mWaiter.Wait();
}

YAGET_COMPILE_SUPRESS_END
