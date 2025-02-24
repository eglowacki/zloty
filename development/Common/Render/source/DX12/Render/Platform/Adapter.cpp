#include "Render/Platform/Adapter.h"
#include "App/AppUtilities.h"
#include "D3D12MemAlloc.h"

#include <d3d12.h>

#include "Core/ErrorHandlers.h"


//-------------------------------------------------------------------------------------------------
yaget::render::platform::Adapter::Adapter([[maybe_unused]] app::WindowFrame windowFrame, const yaget::render::info::Adapter& adapterInfo)
{
    auto [device, adapter, factory] = info::CreateDevice(adapterInfo);
    HRESULT hr = device.As(&mDevice);
    error_handlers::ThrowOnError(hr, "Could not get Device from CreateDevice.");

    hr = adapter.As(&mAdapter);
    error_handlers::ThrowOnError(hr, "Could not get Adapter from CreateDevice.");

    hr = factory.As(&mFactory);
    error_handlers::ThrowOnError(hr, "Could not get Factory from CreateDevice.");

#if YAGET_DEBUG_RENDER == 1
    mDeviceDebugger.ActivateMessageSeverity(mDevice);
#endif // YAGET_DEBUG_RENDER == 1

    D3D12MA::ALLOCATOR_DESC allocatorDesc = {};
    allocatorDesc.pDevice = mDevice.Get();
    allocatorDesc.pAdapter = mAdapter.Get();

    D3D12MA::Allocator* allocator = nullptr;
    hr = D3D12MA::CreateAllocator(&allocatorDesc, &allocator);
    error_handlers::ThrowOnError(hr, "Could not get D3D12MA Allocator");
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

