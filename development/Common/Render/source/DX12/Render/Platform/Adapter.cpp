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
}


//-------------------------------------------------------------------------------------------------
yaget::render::platform::Adapter::~Adapter()
{
}


//-------------------------------------------------------------------------------------------------
Microsoft::WRL::ComPtr<ID3D12Device> yaget::render::platform::Adapter::GetDevice() const
{
    return mDevice;
}


//-------------------------------------------------------------------------------------------------
Microsoft::WRL::ComPtr<IDXGIFactory4> yaget::render::platform::Adapter::GetFactory() const
{
    return mFactory;
}
