#include "PlatformDisplayModes.h"
#include "StringHelpers.h"
#include "RenderHelpers.h"
#include <dxgi.h>

using namespace Microsoft::WRL;

namespace
{
    // return IDXGIFactory used in managing full screen and window mode
    ComPtr<IDXGIFactory> GetDXGIFactory()
    {
        ComPtr<IDXGIFactory> dxgFactory;
        HRESULT hr = ::CreateDXGIFactory(__uuidof(IDXGIFactory), &dxgFactory);
        YAGET_THROW_ON_RROR(hr, "Could not get IDXG Factor from CreateDXGIFactory.");

        return dxgFactory;
    }

    std::vector<ComPtr<IDXGIAdapter>> EnumerateAdapters(IDXGIFactory* dxgFactory)
    {
        std::vector<ComPtr<IDXGIAdapter>> adapters;
        ComPtr<IDXGIAdapter> adapter;

        for (UINT i = 0; dxgFactory->EnumAdapters(i, &adapter) != DXGI_ERROR_NOT_FOUND; ++i)
        {
            adapters.push_back(adapter);
        }

        return adapters;
    }

    // nativeScale - if true, only select resolutions that are even of multiply of monitor resolution
    yaget::render::platform::Adapters GetValidResolutions(IDXGIFactory* dxgFactory, DXGI_FORMAT format, bool nativeScale, uint32_t minimumRefreshRate)
    {
        using namespace yaget;
        render::platform::Adapters supportedModes;

        const uint32_t kMinimumRefreshRate = minimumRefreshRate;

        std::vector<ComPtr<IDXGIAdapter>> adapters = EnumerateAdapters(dxgFactory);

        for (auto it : adapters)
        {
            DXGI_ADAPTER_DESC adapterDesc = {};
            HRESULT hr = it->GetDesc(&adapterDesc);

            std::vector<ComPtr<IDXGIOutput>> vOutputs;
            ComPtr<IDXGIOutput> dxgOutput;

            for (UINT i = 0; it->EnumOutputs(i, &dxgOutput) != DXGI_ERROR_NOT_FOUND; ++i)
            {
                vOutputs.push_back(dxgOutput);
            }

            for (auto out : vOutputs)
            {
                DXGI_OUTPUT_DESC outpuDesc = {};
                hr = out->GetDesc(&outpuDesc);

                render::platform::Adapter adapter;
                adapter.mPlatfrormAdapter = it;
                adapter.mMonitor = outpuDesc.Monitor;
                adapter.mColorFormat = format;
                adapter.AdapterName = conv::wide_to_utf8(adapterDesc.Description);
                adapter.MonitorName = conv::wide_to_utf8(outpuDesc.DeviceName);
                adapter.Width = outpuDesc.DesktopCoordinates.right - outpuDesc.DesktopCoordinates.left;
                adapter.Height = outpuDesc.DesktopCoordinates.bottom - outpuDesc.DesktopCoordinates.top;

                UINT numModes = 0;

                // Get the number of elements
                hr = out->GetDisplayModeList(format, 0, &numModes, nullptr);

                std::vector<DXGI_MODE_DESC> modes(numModes);

                // Get the list
                hr = out->GetDisplayModeList(format, 0/*DXGI_ENUM_MODES_SCALING*/, &numModes, modes.data());

                uint32_t width = adapter.Width;
                uint32_t height = adapter.Height;
                modes.erase(std::remove_if(std::begin(modes), std::end(modes), [width, height, kMinimumRefreshRate, nativeScale](const DXGI_MODE_DESC& modeDesc)
                {
                    uint32_t refreshRate = static_cast<uint32_t>(std::ceil(modeDesc.RefreshRate.Numerator / (modeDesc.RefreshRate.Denominator * 1.0f)));
                    if (refreshRate < kMinimumRefreshRate)
                    {
                        return true;
                    }
                    if (modeDesc.Scaling != DXGI_MODE_SCALING_UNSPECIFIED)
                    {
                        return true;
                    }

                    if (!nativeScale || (nativeScale && (width % modeDesc.Width) == 0 && (height % modeDesc.Height) == 0))
                    {
                        return false;
                    }

                    return true;
                }), std::end(modes));

                for (auto mode : modes)
                {
                    uint32_t refreshRate = static_cast<uint32_t>(std::ceil(mode.RefreshRate.Numerator / (mode.RefreshRate.Denominator * 1.0f)));
                    adapter.Modes.push_back({ mode.Width, mode.Height, refreshRate });
                }

                if (!adapter.Modes.empty())
                {
                    supportedModes.push_back(adapter);
                }
            }
        }

        return supportedModes;
    }

} // namespace


yaget::render::platform::Adapters yaget::render::platform::GetResolutions(DXGI_FORMAT format)
{
    ComPtr<IDXGIFactory> dxgFactory = GetDXGIFactory();
    Adapters adapters = GetValidResolutions(dxgFactory.Get(), format, true, 0);

    return adapters;
}

yaget::render::platform::Resolutions yaget::render::platform::GetResolutions()
{
    Resolutions resolutions;
    ComPtr<IDXGIFactory> dxgFactory = GetDXGIFactory();

    for (int i = 0; i < 133; ++i)
    {
        //render::platform::Adapters adapters = render::platform::GetResolutions(static_cast<DXGI_FORMAT>(i));
        Adapters adapters = GetValidResolutions(dxgFactory.Get(), static_cast<DXGI_FORMAT>(i), true, 0);
        if (!adapters.empty())
        {
            resolutions.insert(std::make_pair(static_cast<DXGI_FORMAT>(i), adapters));
        }
    }

    return resolutions;
}
