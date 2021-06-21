#include "Adapter.h"
#include "App/AppUtilities.h"
#include "StringHelpers.h"

#include <d3d12.h>
#include <dxgi1_4.h>

namespace 
{
    std::string IsSoftware(uint32_t flags)
    {
        return flags & DXGI_ADAPTER_FLAG_SOFTWARE ? "Yes" : "No";
    }
}

//-------------------------------------------------------------------------------------------------
yaget::render::platform::Adapter::Adapter()
{
    using namespace Microsoft::WRL;

    UINT dxgiFactoryFlags = 0;

#if YAGET_DEBUG_RENDER == 1
    dxgiFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
#endif

    HRESULT hr = CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&mFactory));
    YAGET_UTIL_THROW_ON_RROR(hr, "Could not create DX12 Factory2");

    // collect all valid adapters
    std::vector<ComPtr<IDXGIAdapter1>> foundAdapters;
    ComPtr<IDXGIAdapter1> tempAdapter;
    for (UINT adapterIndex = 0; DXGI_ERROR_NOT_FOUND != mFactory->EnumAdapters1(adapterIndex, &tempAdapter); ++adapterIndex)
    {
        if (SUCCEEDED(D3D12CreateDevice(tempAdapter.Get(), D3D_FEATURE_LEVEL_12_0, _uuidof(ID3D12Device), nullptr)))
        {
            foundAdapters.push_back(tempAdapter);
        }
    }

    // chose which one to select
    for (const auto& adapter : foundAdapters)
    {
        DXGI_ADAPTER_DESC1 desc;
        adapter->GetDesc1(&desc);

        YLOG_NOTICE("DEVI", "Adapter: Name: '%s', Software: '%s', VendorId: '%d', DeviceId: '%d', Video Memory: '%s' bytes, System Memory: '%s' bytes, Flags: '%d'.",
            conv::wide_to_utf8(desc.Description).c_str(), IsSoftware(desc.Flags).c_str(), desc.VendorId, desc.DeviceId, 
            conv::ToThousandsSep(desc.DedicatedVideoMemory).c_str(), conv::ToThousandsSep(desc.SharedSystemMemory).c_str(), desc.Flags);

        if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
        {
            // Don't select the Basic Render Driver adapter.
            continue;
        }

        // use this adapter, unless later ones will have a better one
        mAdapter = adapter;
    }

    YAGET_UTIL_THROW_ON_RROR(mAdapter, "Could not get DX12 Adapter1 interface");

    hr = D3D12CreateDevice(mAdapter.Get(), D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&mDevice));
    YAGET_UTIL_THROW_ON_RROR(hr, "Could not create DX12 Device");

    mDevice->SetName(L"Yaget Device");

#if YAGET_DEBUG_RENDER == 1
    hr = mDevice->QueryInterface(IID_PPV_ARGS(&mDebugDevice));
    YAGET_UTIL_THROW_ON_RROR(hr, "Could not get DX12 Debug Device Interface");
#endif
}


//-------------------------------------------------------------------------------------------------
yaget::render::platform::Adapter::~Adapter()
{
#if YAGET_DEBUG_RENDER == 1
    mDevice = nullptr;

    const D3D12_RLDO_FLAGS flags = D3D12_RLDO_DETAIL | D3D12_RLDO_IGNORE_INTERNAL;
    mDebugDevice->ReportLiveDeviceObjects(flags);
#endif
}


//-------------------------------------------------------------------------------------------------
const Microsoft::WRL::ComPtr<ID3D12Device>& yaget::render::platform::Adapter::GetDevice() const
{
    return mDevice;
}


//-------------------------------------------------------------------------------------------------
const Microsoft::WRL::ComPtr<IDXGIFactory4>& yaget::render::platform::Adapter::GetFactory() const
{
    return mFactory;
}


//-------------------------------------------------------------------------------------------------
yaget::render::platform::CommandQueue::CommandQueue(const Microsoft::WRL::ComPtr<ID3D12Device>& device)
{
    using namespace Microsoft::WRL;

    D3D12_COMMAND_QUEUE_DESC queueDesc = {};
    queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

    HRESULT hr = device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&mCommandQueue));
    YAGET_UTIL_THROW_ON_RROR(hr, "Could not create DX12 Command Queue");

    mCommandQueue->SetName(L"Yaget CommandQueue");
}


//-------------------------------------------------------------------------------------------------
yaget::render::platform::CommandQueue::~CommandQueue()
{
}


//-------------------------------------------------------------------------------------------------
yaget::render::platform::Fence::Fence(const ComPtr<ID3D12Device>& device)
{
    HRESULT hr = device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&mFence));
    YAGET_UTIL_THROW_ON_RROR(hr, "Could not create DX12 Fence");

    mFenceEvent = ::CreateEvent(nullptr, FALSE, FALSE, nullptr);
    YAGET_UTIL_THROW_ON_RROR(mFenceEvent, "Could not create Event");

    mFenceValue = 1;
}


//-------------------------------------------------------------------------------------------------
yaget::render::platform::Fence::~Fence()
{
    ::CloseHandle(mFenceEvent);
}


//-------------------------------------------------------------------------------------------------
void yaget::render::platform::Fence::Wait(CommandQueue& commandQueue)
{
    // WAITING FOR THE FRAME TO COMPLETE BEFORE CONTINUING IS NOT BEST PRACTICE.
    // This is code implemented as such for simplicity. The D3D12HelloFrameBuffering
    // sample illustrates how to use fences for efficient resource usage and to
    // maximize GPU utilization.

    // Signal and increment the fence value.
    const UINT64 fence = mFenceValue;
    HRESULT hr = commandQueue.Get()->Signal(mFence.Get(), fence);
    YAGET_UTIL_THROW_ON_RROR(hr, "Could not signal DX12 Command Queue with Fence");

    mFenceValue++;

    // Wait until the previous frame is finished.
    if (mFence->GetCompletedValue() < fence)
    {
        hr = mFence->SetEventOnCompletion(fence, mFenceEvent);
        YAGET_UTIL_THROW_ON_RROR(hr, "Could not set DX12 Fence Event On Completion");

        ::WaitForSingleObject(mFenceEvent, INFINITE);
    }
}
