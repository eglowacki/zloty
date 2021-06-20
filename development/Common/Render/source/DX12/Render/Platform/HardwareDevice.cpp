#include "HardwareDevice.h"
#include "App/AppUtilities.h"

#include <d3d12.h>
#include <dxgi1_6.h>


#if 0
namespace
{
    using namespace Microsoft::WRL;

    //-------------------------------------------------------------------------------------------------
    ComPtr<ID3D12Debug1> EnableDebugLayer()
    {
#ifdef YAGET_DEBUG
        // Always enable the debug layer before doing anything DX12 related
        // so all possible errors generated while creating DX12 objects
        // are caught by the debug layer.
        ComPtr<ID3D12Debug> debugInterface;
        HRESULT hr = D3D12GetDebugInterface(IID_PPV_ARGS(&debugInterface));
        YAGET_UTIL_THROW_ON_RROR(hr, "Could not get DX12 DebugInterface");

        ComPtr<ID3D12Debug1> debugController;
        hr = debugInterface->QueryInterface(IID_PPV_ARGS(&debugController));
        YAGET_UTIL_THROW_ON_RROR(hr, "Could not get DX12 DebugController");

        debugController->EnableDebugLayer();
        debugController->SetEnableGPUBasedValidation(true);

        return debugController;
#endif // YAGET_DEBUG

        return nullptr;
    }

    //-------------------------------------------------------------------------------------------------
    ComPtr<IDXGIAdapter4> GetAdapter(bool useWarp)
    {
        Microsoft::WRL::ComPtr<IDXGIFactory4> dxgiFactory;
        UINT createFactoryFlags = 0;
#ifdef YAGET_DEBUG
        createFactoryFlags = DXGI_CREATE_FACTORY_DEBUG;
#endif // YAGET_DEBUG

        HRESULT hr = CreateDXGIFactory2(createFactoryFlags, IID_PPV_ARGS(&dxgiFactory));
        YAGET_UTIL_THROW_ON_RROR(hr, "Could not get GXGI Factory2");

        ComPtr<IDXGIAdapter1> dxgiAdapter1;
        ComPtr<IDXGIAdapter4> dxgiAdapter4;

        if (useWarp)
        {
            hr = dxgiFactory->EnumWarpAdapter(IID_PPV_ARGS(&dxgiAdapter1));
            YAGET_UTIL_THROW_ON_RROR(hr, "Could not enumerate DX12 adapters");

            hr = dxgiAdapter1.As(&dxgiAdapter4);
            YAGET_UTIL_THROW_ON_RROR(hr, "Could not get DX12 addapter interface 4");
        }
        else
        {
            SIZE_T maxDedicatedVideoMemory = 0;
            for (UINT i = 0; dxgiFactory->EnumAdapters1(i, &dxgiAdapter1) != DXGI_ERROR_NOT_FOUND; ++i)
            {
                DXGI_ADAPTER_DESC1 dxgiAdapterDesc1;
                dxgiAdapter1->GetDesc1(&dxgiAdapterDesc1);

                // Check to see if the adapter can create a D3D12 device without actually 
                // creating it. The adapter with the largest dedicated video memory
                // is favored.
                if ((dxgiAdapterDesc1.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) == 0 &&
                    SUCCEEDED(D3D12CreateDevice(dxgiAdapter1.Get(), D3D_FEATURE_LEVEL_12_0, __uuidof(ID3D12Device), nullptr)) &&
                    dxgiAdapterDesc1.DedicatedVideoMemory > maxDedicatedVideoMemory)
                {
                    maxDedicatedVideoMemory = dxgiAdapterDesc1.DedicatedVideoMemory;
                    hr = dxgiAdapter1.As(&dxgiAdapter4);
                }
            }
        }

        return dxgiAdapter4;
    }

    //-------------------------------------------------------------------------------------------------
    ComPtr<ID3D12Device2> CreateDevice(ComPtr<IDXGIAdapter4> adapter)
    {
        ComPtr<ID3D12Device2> d3d12Device2;
        HRESULT hr = D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&d3d12Device2));
        YAGET_UTIL_THROW_ON_RROR(hr, "Could not create D12 Device");

        // Enable debug messages in debug mode.
#ifdef YAGET_DEBUG
        ComPtr<ID3D12InfoQueue> pInfoQueue;
        if (SUCCEEDED(d3d12Device2.As(&pInfoQueue)))
        {
            pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, TRUE);
            pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, TRUE);
            pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, TRUE);

            // Suppress whole categories of messages
            //D3D12_MESSAGE_CATEGORY Categories[] = {};

            // Suppress messages based on their severity level
            D3D12_MESSAGE_SEVERITY Severities[] =
            {
                D3D12_MESSAGE_SEVERITY_INFO
            };

            // Suppress individual messages by their ID
            D3D12_MESSAGE_ID DenyIds[] = {
                D3D12_MESSAGE_ID_CLEARRENDERTARGETVIEW_MISMATCHINGCLEARVALUE,   // I'm really not sure how to avoid this message.
                D3D12_MESSAGE_ID_MAP_INVALID_NULLRANGE,                         // This warning occurs when using capture frame while graphics debugging.
                D3D12_MESSAGE_ID_UNMAP_INVALID_NULLRANGE,                       // This warning occurs when using capture frame while graphics debugging.
            };

            D3D12_INFO_QUEUE_FILTER NewFilter = {};
            //NewFilter.DenyList.NumCategories = _countof(Categories);
            //NewFilter.DenyList.pCategoryList = Categories;
            NewFilter.DenyList.NumSeverities = _countof(Severities);
            NewFilter.DenyList.pSeverityList = Severities;
            NewFilter.DenyList.NumIDs = _countof(DenyIds);
            NewFilter.DenyList.pIDList = DenyIds;

            hr = pInfoQueue->PushStorageFilter(&NewFilter);
            YAGET_UTIL_THROW_ON_RROR(hr, "Could not push DX12 Storage Filter");
        }
#endif // YAGET_DEBUG

        return d3d12Device2;
    }

    //-------------------------------------------------------------------------------------------------
    ComPtr<ID3D12Device2> CreateDevice()
    {
        auto adapter = GetAdapter(false);
        return CreateDevice(adapter);
    }

    //-------------------------------------------------------------------------------------------------
    ComPtr<ID3D12CommandQueue> CreateCommandQueue(ComPtr<ID3D12Device2> device, D3D12_COMMAND_LIST_TYPE type)
    {
        ComPtr<ID3D12CommandQueue> d3d12CommandQueue;

        D3D12_COMMAND_QUEUE_DESC desc = {};
        desc.Type = type;
        desc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
        desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
        desc.NodeMask = 0;

        HRESULT hr = device->CreateCommandQueue(&desc, IID_PPV_ARGS(&d3d12CommandQueue));
        YAGET_UTIL_THROW_ON_RROR(hr, "Could not create DX12 Command Queue");

        return d3d12CommandQueue;
    }

}


//-------------------------------------------------------------------------------------------------
yaget::render::platform::HardwareDevice::HardwareDevice(Application& app)
    : mDevice(CreateDevice())
    , mSwapChain(app, mDevice.Get(), 3)
#ifdef YAGET_DEBUG
    , mDebugDevice(EnableDebugLayer())
#endif // YAGET_DEBUG
{
}


//-------------------------------------------------------------------------------------------------
yaget::render::platform::HardwareDevice::~HardwareDevice()
{
}


//-------------------------------------------------------------------------------------------------
void yaget::render::platform::HardwareDevice::Render(const time::GameClock& gameClock, metrics::Channel& channel)
{
    mSwapChain.Render(gameClock, channel);
}


//-------------------------------------------------------------------------------------------------
void yaget::render::platform::HardwareDevice::Resize()
{
    mSwapChain.Resize();
}
#endif