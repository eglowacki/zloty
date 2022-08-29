#include "Render/AdapterInfo.h"
#include "App/AppUtilities.h"
#include "Debugging/DevConfiguration.h"
#include "Render/Platform/DeviceDebugger.h"

#include <d3d12.h>
#include <dxgi1_6.h>

#include <comdef.h>

//-------------------------------------------------------------------------------------------------
namespace 
{
    using namespace yaget::render;

    std::string IsSoftware(uint32_t flags)
    {
        return flags & DXGI_ADAPTER_FLAG_SOFTWARE ? "Yes" : "No";
    }
    
    D3D_FEATURE_LEVEL DXfeatureLevels[] = 
    {
        D3D_FEATURE_LEVEL_12_2,
        D3D_FEATURE_LEVEL_12_1,
        D3D_FEATURE_LEVEL_12_0
    };

    ComPtr<IDXGIFactory7> CreateFactory()
    {
        UINT dxgiFactoryFlags = 0;

#if YAGET_DEBUG_RENDER == 1
        dxgiFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
#endif // YAGET_DEBUG_RENDER == 1

        ComPtr<IDXGIFactory2> compitableFactory{};
        HRESULT hr = ::CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&compitableFactory));
        YAGET_UTIL_THROW_ON_RROR(hr, "Could not create DX12 Factory");

        ComPtr<IDXGIFactory7> factory{};
        hr = compitableFactory.As(&factory);
        YAGET_UTIL_THROW_ON_RROR(hr, "Could not get factory Interface 7 from from DXGI factory.");

        return factory;
    }

} // namespace


//-------------------------------------------------------------------------------------------------
yaget::render::info::Filters yaget::render::info::GetDefaultFilters()
{
    yaget::render::info::Filters filters{
        [](auto level)
        {   
            return level >= D3D_FEATURE_LEVEL_12_0;
        },
        nullptr,
        nullptr,
        [](auto format)
        {
            return format == DXGI_FORMAT_R8G8B8A8_UNORM;
        },
        [](auto resolution)
        {
            return resolution.mRefreshRate >= 60;
        }
    };

    return filters;
}


//-------------------------------------------------------------------------------------------------
yaget::render::info::Adapters yaget::render::info::EnumerateAdapters(Filters filters, bool referenceRasterizer)
{
    Adapters adapters;
    ComPtr<IDXGIFactory7> factory = CreateFactory();

    std::vector<ComPtr<IDXGIAdapter1>> foundAdapters{};

    if (referenceRasterizer)
    {
        ComPtr<IDXGIAdapter1> adapter{};
        HRESULT hr = factory->EnumWarpAdapter(IID_PPV_ARGS(&adapter));
        YAGET_UTIL_THROW_ON_RROR(hr, "Could not enumerate warp adapter");

        foundAdapters.emplace_back(adapter);
    }
    else
    {
        ComPtr<IDXGIAdapter1> adapter;
        for (UINT adapterIndex = 0; DXGI_ERROR_NOT_FOUND != factory->EnumAdapters1(adapterIndex, &adapter); ++adapterIndex)
        {
            DXGI_ADAPTER_DESC1 desc;
            HRESULT hr = adapter->GetDesc1(&desc);
            YAGET_UTIL_THROW_ON_RROR(hr, "Could not get DX12 adapter descritpion");

            if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
            {
                continue;
            }

            foundAdapters.emplace_back(adapter);
        }
    }

    for (const auto& adapter : foundAdapters)
    {
        for (const auto featureLevel : DXfeatureLevels)
        {
            // do we want to this feature level?
            if (!filters.IsFeatureLevel(featureLevel))
            {
                break;
            }

            HRESULT hr = D3D12CreateDevice(adapter.Get(), featureLevel, _uuidof(ID3D12Device2), nullptr);
            if (SUCCEEDED(hr))
            {
                DXGI_ADAPTER_DESC1 desc;
                hr = adapter->GetDesc1(&desc);
                YAGET_UTIL_THROW_ON_RROR(hr, "Could not get DX12 adapter descritpion");

                // do we want to keep this adapter?
                if (!filters.IsAdapter(conv::wide_to_utf8(desc.Description)))
                {
                    continue;
                }

                // let's get supported resolutions for this adapter
                ComPtr<IDXGIOutput> output;
                std::vector<Output> collectedOutputs;
                for (UINT outputIndex = 0; adapter->EnumOutputs(outputIndex, &output) != DXGI_ERROR_NOT_FOUND; ++outputIndex)
                {
                    DXGI_OUTPUT_DESC outputDesc{};
                    hr =  output->GetDesc(&outputDesc);
                    YAGET_UTIL_THROW_ON_RROR(hr, "Could not get adapter output descritpion");

                    // do we want to keep this output?
                    if (!filters.IsOutput(conv::wide_to_utf8(outputDesc.DeviceName)))
                    {
                        continue;
                    }

                    Output monitorOutput{conv::wide_to_utf8(outputDesc.DeviceName), outputDesc.Monitor};

                    for (int i = 0; i < 191; ++i)
                    {
                        DXGI_FORMAT currentFormat = static_cast<DXGI_FORMAT>(i);
                        UINT numModes = 0;

                        hr = output->GetDisplayModeList(currentFormat, 0, &numModes, nullptr);
                        YAGET_UTIL_THROW_ON_RROR(hr, "Could not get number of display modes from adapter");

                        if (numModes)
                        {
                            // do we want to keep this format?
                            if (!filters.IsFormat(currentFormat))
                            {
                                continue;
                            }

                            std::vector<DXGI_MODE_DESC> displayModes;
                            displayModes.resize(numModes);

                            hr = output->GetDisplayModeList(currentFormat, 0, &numModes, displayModes.data());
                            YAGET_UTIL_THROW_ON_RROR(hr, "Could not get Display Mode List from adapter");

                            //Strings resolutions;
                            for (const auto& display: displayModes)
                            {
                                if (display.Scaling != DXGI_MODE_SCALING_UNSPECIFIED)
                                {
                                    continue;
                                }

                                const auto refreshRate = std::lround(display.RefreshRate.Numerator / (display.RefreshRate.Denominator * 1.0f));
                                const Resolution resolution{display.Width, display.Height, refreshRate, currentFormat};

                                if (!filters.IsResolution(resolution))
                                {
                                    continue;
                                }

                                monitorOutput.mResolutions.emplace_back(resolution);
                            }
                        }
                    }

                    if (!monitorOutput.mResolutions.empty())
                    {
                        collectedOutputs.push_back(monitorOutput);
                    }
                }

                YLOG_NOTICE("DEVI", "Found Adapter: Name: '%s', Software: '%s', VendorId: '%d', DeviceId: '%d', Video Memory: '%s' bytes, System Memory: '%s' bytes, Flags: '%d'.",
                    conv::wide_to_utf8(desc.Description).c_str(), IsSoftware(desc.Flags).c_str(), desc.VendorId, desc.DeviceId,
                    conv::ToThousandsSep(desc.DedicatedVideoMemory).c_str(), conv::ToThousandsSep(desc.SharedSystemMemory).c_str(), desc.Flags);

                adapters.emplace_back(Adapter{conv::wide_to_utf8(desc.Description), static_cast<bool>(desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE), desc.DedicatedVideoMemory, desc.AdapterLuid, featureLevel, collectedOutputs});
                break;
            }
        }
    }

    return adapters;
}


yaget::render::info::Adapter yaget::render::info::SelectAdapter(const Adapters& adapters, Filters filters)
{
    for (const auto& adapter: adapters)
    {
        if (!filters.IsFeatureLevel(adapter.mFeatureLevel) || !filters.IsAdapter(adapter.mName))
        {
            continue;
        }

        for (const auto& output: adapter.mOutputs)
        {
            if (!filters.IsOutput(output.mName))
            {
                continue;
            }

            for (const auto& resolution: output.mResolutions)
            {
                if (!filters.IsFormat(resolution.mFormat))
                {
                    continue;
                }

                if (!filters.IsResolution(resolution))
                {
                    continue;
                }

                Adapter selectedAdapter = adapter;

                selectedAdapter.mOutputs = { output };
                (*selectedAdapter.mOutputs.begin()).mResolutions = { resolution };

                return selectedAdapter;
            }
        }
    }

    return {};
}


yaget::render::info::HardwareDevice yaget::render::info::CreateDevice(const Adapter& adapter)
{
    ComPtr<IDXGIFactory7> factory = CreateFactory();

    ComPtr<IDXGIAdapter4> hardwareAdapter;
    HRESULT hr = factory->EnumAdapterByLuid(adapter.mId, IID_PPV_ARGS(&hardwareAdapter));
    YAGET_UTIL_THROW_ON_RROR(hr, "Could not get requested Hardware Adpater");

    ComPtr<ID3D12Device2> device;
    hr = D3D12CreateDevice(hardwareAdapter.Get(), D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&device));
    YAGET_UTIL_THROW_ON_RROR(hr, "Could not create DX12 Device");

    ComPtr<ID3D12Device7> hardwareDevice{};
    hr = device->QueryInterface<ID3D12Device7>(&hardwareDevice);
    YAGET_UTIL_THROW_ON_RROR(hr, "Could not get DX12 Device4 interface");

    YAGET_RENDER_SET_DEBUG_NAME(hardwareDevice.Get(), "Yaget Device");

    D3D12_FEATURE_DATA_FEATURE_LEVELS featureLevels{};
    featureLevels.NumFeatureLevels = 1;
    D3D_FEATURE_LEVEL requested[] = { static_cast<D3D_FEATURE_LEVEL>(adapter.mFeatureLevel) };
    featureLevels.pFeatureLevelsRequested = requested;

    hr = hardwareDevice->CheckFeatureSupport(D3D12_FEATURE_FEATURE_LEVELS, &featureLevels, sizeof(featureLevels));
    YAGET_UTIL_THROW_ON_RROR(hr, "Could not check feature support");
    YAGET_UTIL_THROW_ASSERT("DEVI", featureLevels.MaxSupportedFeatureLevel >= adapter.mFeatureLevel, "Supported feature level is not compitable with requested.");

    return { hardwareDevice, hardwareAdapter, factory };
}
