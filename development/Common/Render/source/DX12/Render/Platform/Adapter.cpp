#include "Render/Platform/Adapter.h"
#include "App/AppUtilities.h"
#include "D3D12MemAlloc.h"

#include <d3d12.h>


//-------------------------------------------------------------------------------------------------
yaget::render::platform::Adapter::Adapter([[maybe_unused]] app::WindowFrame windowFrame, const yaget::render::info::Adapter& adapterInfo)
{
    auto [device, adapter, factory] = info::CreateDevice(adapterInfo);
    HRESULT hr = device.As(&mDevice);
    YAGET_UTIL_THROW_ON_RROR(hr, "Could not get Device from CreateDevice.");

    hr = adapter.As(&mAdapter);
    YAGET_UTIL_THROW_ON_RROR(hr, "Could not get Adapter from CreateDevice.");

    hr = factory.As(&mFactory);
    YAGET_UTIL_THROW_ON_RROR(hr, "Could not get Factory from CreateDevice.");

#if YAGET_DEBUG_RENDER == 1
    mDeviceDebugger.ActivateMessageSeverity(mDevice);
#endif // YAGET_DEBUG_RENDER == 1

    D3D12MA::ALLOCATOR_DESC allocatorDesc = {};
    allocatorDesc.pDevice = mDevice.Get();
    allocatorDesc.pAdapter = mAdapter.Get();

    D3D12MA::Allocator* allocator = nullptr;
    hr = D3D12MA::CreateAllocator(&allocatorDesc, &allocator);
    YAGET_UTIL_THROW_ON_RROR(hr, "Could not get D3D12MA Allocator");
    mAllocator.reset(allocator);
}


//-------------------------------------------------------------------------------------------------
yaget::render::platform::Adapter::~Adapter() = default;


//-------------------------------------------------------------------------------------------------
ID3D12Device* yaget::render::platform::Adapter::GetDevice() const
{
    return mDevice.Get();
}


//-------------------------------------------------------------------------------------------------
IDXGIFactory* yaget::render::platform::Adapter::GetFactory() const
{
    return mFactory.Get();
}


//-------------------------------------------------------------------------------------------------
D3D12MA::Allocator* yaget::render::platform::Adapter::GetAllocator() const
{
    return mAllocator.get();
}

