#include "Render/Device.h"
#include "App/AppUtilities.h"
#include "Metrics/Concurrency.h"
#include "Platform/Adapter.h"
#include "Platform/Support.h"
#include "Platform/CommandQueue.h"
#include "Render/Platform/SwapChain.h"
#include "StringHelpers.h"
#include "Time/GameClock.h"

#include <dxgi1_6.h>

#include "Debugging/DevConfiguration.h"


namespace
{
    //-------------------------------------------------------------------------------------------------
    std::vector<char> ReadFile(const std::string& filename)
    {
        std::ifstream file(filename, std::ios::ate | std::ios::binary);
        //bool exists = (bool)file;

        if (!file || !file.is_open())
        {
            throw std::runtime_error("failed to open file!");
        }

        const size_t fileSize = file.tellg();
        std::vector<char> buffer(fileSize);

        file.seekg(0);
        file.read(buffer.data(), fileSize);

        return buffer;
    };

    //-------------------------------------------------------------------------------------------------
    struct Vertex
    {
        DirectX::XMFLOAT3 position;
        DirectX::XMFLOAT4 color;
    };

}


//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
yaget::render::DeviceB::DeviceB(app::WindowFrame windowFrame)
    : mWindowFrame{ windowFrame }
    , mAdapter{ std::make_unique<platform::Adapter>(windowFrame) }
    , mSwapChain{ std::make_unique<platform::SwapChain>(windowFrame, mAdapter->GetDevice(), mAdapter->GetFactory().Get()) }
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
int64_t yaget::render::DeviceB::OnHandleRawInput(void* hWnd, uint32_t message, uint64_t wParam, int64_t lParam)
{
    return 0;
}


//-------------------------------------------------------------------------------------------------
void yaget::render::DeviceB::RenderFrame(const time::GameClock& gameClock, metrics::Channel& channel)
{
    mSwapChain->Render();

    //constexpr time::TimeUnits_t waitTime = 1;
    //constexpr time::TimeUnits_t unitType = time::kMilisecondUnit;
    //yaget::platform::Sleep(waitTime, unitType);

    mWaiter.Wait();
}

YAGET_COMPILE_SUPRESS_END
