#include "Adapter.h"
#include "App/AppUtilities.h"
#include "D3D12MemAlloc.h"
#include "Debugging/DevConfiguration.h"
#include "Render/RenderStringHelpers.h"
#include "SwapChain.h"

#include <d3d12.h>
#include <dxgi1_6.h>


//-------------------------------------------------------------------------------------------------
namespace 
{
    std::string IsSoftware(uint32_t flags)
    {
        return flags & DXGI_ADAPTER_FLAG_SOFTWARE ? "Yes" : "No";
    }

}


//-------------------------------------------------------------------------------------------------
yaget::render::platform::Adapter::Adapter([[maybe_unused]] app::WindowFrame windowFrame, [[maybe_unused]] bool useDefault /*= false*/)
{
    using namespace Microsoft::WRL;

    UINT dxgiFactoryFlags = 0;

#if YAGET_DEBUG_RENDER == 1
    dxgiFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
#endif // YAGET_DEBUG_RENDER == 1

    HRESULT hr = CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&mFactory));
    YAGET_UTIL_THROW_ON_RROR(hr, "Could not create DX12 Factory2");

    if (yaget::dev::CurrentConfiguration().mInit.SoftwareRender)
    {
        ComPtr<IDXGIAdapter1> adapter;
        mFactory->EnumWarpAdapter(IID_PPV_ARGS(&adapter));
        adapter.As(&mAdapter);

        DXGI_ADAPTER_DESC1 desc;
        hr = adapter->GetDesc1(&desc);
        YAGET_UTIL_THROW_ON_RROR(hr, "Could not get DX12 warp adapter descritpion");

        YLOG_NOTICE("DEVI", "Found Warp Adapter: Name: '%s', Software: '%s', VendorId: '%d', DeviceId: '%d', Video Memory: '%s' bytes, System Memory: '%s' bytes, Flags: '%d'.",
            conv::wide_to_utf8(desc.Description).c_str(), IsSoftware(desc.Flags).c_str(), desc.VendorId, desc.DeviceId,
            conv::ToThousandsSep(desc.DedicatedVideoMemory).c_str(), conv::ToThousandsSep(desc.SharedSystemMemory).c_str(), desc.Flags);
    }
    else
    {
        // collect all valid adapters
        std::vector<ComPtr<IDXGIAdapter1>> foundAdapters;
        ComPtr<IDXGIAdapter1> tempAdapter;
        for (UINT adapterIndex = 0; DXGI_ERROR_NOT_FOUND != mFactory->EnumAdapters1(adapterIndex, &tempAdapter); ++adapterIndex)
        {
            if (SUCCEEDED(D3D12CreateDevice(tempAdapter.Get(), D3D_FEATURE_LEVEL_12_0, _uuidof(ID3D12Device2), nullptr)))
            {
                DXGI_ADAPTER_DESC1 desc;
                hr = tempAdapter->GetDesc1(&desc);
                YAGET_UTIL_THROW_ON_RROR(hr, "Could not get DX12 adapter descritpion");

                YLOG_NOTICE("DEVI", "Found Adapter: Name: '%s', Software: '%s', VendorId: '%d', DeviceId: '%d', Video Memory: '%s' bytes, System Memory: '%s' bytes, Flags: '%d'.",
                    conv::wide_to_utf8(desc.Description).c_str(), IsSoftware(desc.Flags).c_str(), desc.VendorId, desc.DeviceId,
                    conv::ToThousandsSep(desc.DedicatedVideoMemory).c_str(), conv::ToThousandsSep(desc.SharedSystemMemory).c_str(), desc.Flags);

                foundAdapters.push_back(tempAdapter);
            }
        }

        // chose which one to select
        for (const auto& adapter : foundAdapters)
        {
            DXGI_ADAPTER_DESC1 desc;
            hr = adapter->GetDesc1(&desc);
            YAGET_UTIL_THROW_ON_RROR(hr, "Could not get DX12 adapter descritpion");

            if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
            {
                // Don't select the Basic Render Driver adapter.
                continue;
            }

            YLOG_NOTICE("DEVI", "Selected Adapter: Name: '%s', Software: '%s', VendorId: '%d', DeviceId: '%d', Video Memory: '%s' bytes, System Memory: '%s' bytes, Flags: '%d'.",
                conv::wide_to_utf8(desc.Description).c_str(), IsSoftware(desc.Flags).c_str(), desc.VendorId, desc.DeviceId,
                conv::ToThousandsSep(desc.DedicatedVideoMemory).c_str(), conv::ToThousandsSep(desc.SharedSystemMemory).c_str(), desc.Flags);

            // use this adapter, unless later ones will have a better one
            adapter.As(&mAdapter);
        }
    }

    YAGET_UTIL_THROW_ON_RROR(mAdapter, "Could not get DX12 Adapter1 interface");

    ComPtr<ID3D12Device2> device;
    hr = D3D12CreateDevice(mAdapter.Get(), D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&device));
    YAGET_UTIL_THROW_ON_RROR(hr, "Could not create DX12 Device");

    hr = device->QueryInterface<ID3D12Device4>(&mDevice);
    YAGET_UTIL_THROW_ON_RROR(hr, "Could not get DX12 Device4 interface");

    YAGET_RENDER_SET_DEBUG_NAME(mDevice.Get(), "Yaget Device");

    // print feature level
    // TODO: make some data structure that holds various feature levels at run time for current graphics setup.
    D3D12_FEATURE_DATA_FEATURE_LEVELS feature_level{};
    feature_level.NumFeatureLevels = 3;
    D3D_FEATURE_LEVEL requested[3] = {D3D_FEATURE_LEVEL_12_2, D3D_FEATURE_LEVEL_12_1, D3D_FEATURE_LEVEL_12_0};
    feature_level.pFeatureLevelsRequested = requested;

    hr = mDevice->CheckFeatureSupport(D3D12_FEATURE_FEATURE_LEVELS, &feature_level, sizeof(feature_level));
    YAGET_UTIL_THROW_ON_RROR(hr, "Could not check feature support");

    const auto maxFeatureLevelSupported = conv::Convertor<D3D_FEATURE_LEVEL>::ToString(feature_level.MaxSupportedFeatureLevel);
    YLOG_NOTICE("DEVI", "Device supports: '%s' set.", conv::Convertor<D3D_FEATURE_LEVEL>::ToString(feature_level.MaxSupportedFeatureLevel).c_str());

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
const Microsoft::WRL::ComPtr<ID3D12Device4>& yaget::render::platform::Adapter::GetDevice() const
{
    return mDevice;
}


//-------------------------------------------------------------------------------------------------
const Microsoft::WRL::ComPtr<IDXGIFactory4>& yaget::render::platform::Adapter::GetFactory() const
{
    return mFactory;
}


//-------------------------------------------------------------------------------------------------
D3D12MA::Allocator* yaget::render::platform::Adapter::GetAllocator() const
{
    return mAllocator.get();
}


//-------------------------------------------------------------------------------------------------
void yaget::render::platform::Adapter::Deleter::operator()(D3D12MA::Allocator* allocator) const
{
    if (allocator)
    {
        allocator->Release();
    }
}
