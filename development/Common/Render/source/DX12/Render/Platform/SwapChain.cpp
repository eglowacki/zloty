#include "Render/Platform/SwapChain.h"
#include "StringHelpers.h"
#include "Render/Polygons/Polygon.h"
#include "Render/Platform/DeviceDebugger.h"
#include "Render/Metrics/RenderMetrics.h"
#include "Render/AdapterInfo.h"
#include "App/AppUtilities.h"
#include "App/Application.h"
#include "CommandQueue.h"
#include "MathFacade.h"

#include <d3dx12.h>
#include <dxgi1_6.h>

#include "Core/ErrorHandlers.h"


namespace 
{
    //-------------------------------------------------------------------------------------------------
    bool CheckTearingSupport(const Microsoft::WRL::ComPtr<IDXGIFactory>& factory)
    {
        BOOL allowTearing = FALSE;

        Microsoft::WRL::ComPtr<IDXGIFactory5> factory5;
        HRESULT hr = factory.As(&factory5);
        if (SUCCEEDED(hr))
        {
            hr = factory5->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING, &allowTearing, sizeof(allowTearing));
            if (FAILED(hr))
            {
                allowTearing = FALSE;
            }
        }

        return allowTearing == TRUE;
    }

    //-------------------------------------------------------------------------------------------------
    yaget::render::ComPtr<IDXGISwapChain4> CreateSwapChain(const yaget::app::WindowFrame& windowFrame, const yaget::render::info::Adapter& adapterInfo, IDXGIFactory* factory, ID3D12CommandQueue* commandQueue, uint32_t numBackBuffers, bool tearingSupported)
    {
        const auto& adapterResolution = adapterInfo.GetSelectedResolution();

        DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
        swapChainDesc.BufferCount = numBackBuffers;
        swapChainDesc.Width = 0;
        swapChainDesc.Height = 0;
        swapChainDesc.Format = static_cast<DXGI_FORMAT>(adapterResolution.mFormat);
        swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
        swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
        swapChainDesc.SampleDesc = { 1, 0 };
        swapChainDesc.Flags = (tearingSupported ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0) /*| DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH*/;

        yaget::render::ComPtr<IDXGIFactory> baseFactory(factory);
        yaget::render::ComPtr<IDXGIFactory2> factory2;
        HRESULT hr = baseFactory.As(&factory2);
        yaget::error_handlers::ThrowOnError(hr, "Could not get factory 2 interface");

        yaget::render::ComPtr<IDXGISwapChain1> swapChain;
        hr = factory2->CreateSwapChainForHwnd(commandQueue, windowFrame.GetSurface().Handle<HWND>(), &swapChainDesc, nullptr, nullptr, &swapChain);
        yaget::error_handlers::ThrowOnError(hr, "Could not create DX12 SwapChain");

        // Disable the Alt+Enter fullscreen toggle feature. Switching to fullscreen will be handled manually.
        hr = factory2->MakeWindowAssociation(windowFrame.GetSurface().Handle<HWND>(), DXGI_MWA_NO_ALT_ENTER);
        yaget::error_handlers::ThrowOnError(hr, "Could not make DX12 Window Association");

        yaget::render::ComPtr<IDXGISwapChain4> swapChain4;
        hr = swapChain.As(&swapChain4);
        yaget::error_handlers::ThrowOnError(hr, "Could not get DX12 SwapChain4 interface");

        return swapChain4;
    }

    //-------------------------------------------------------------------------------------------------
    yaget::render::ComPtr<ID3D12DescriptorHeap> CreateDescriptorHeap(ID3D12Device* device, D3D12_DESCRIPTOR_HEAP_TYPE type, uint32_t numDescriptors)
    {
        D3D12_DESCRIPTOR_HEAP_DESC desc = {};
        desc.NumDescriptors = numDescriptors;
        desc.Type = type;
    
        yaget::render::ComPtr<ID3D12DescriptorHeap> descriptorHeap;
        const HRESULT hr = device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&descriptorHeap));
        yaget::error_handlers::ThrowOnError(hr, "Could not create DX12 DescriptorHeap");

        YAGET_RENDER_SET_DEBUG_NAME(descriptorHeap.Get(), "Yaget Descriptor Heap");

        return descriptorHeap;
    }

    //-------------------------------------------------------------------------------------------------
    yaget::render::ComPtr<ID3D12GraphicsCommandList2> CreateCommandList(ID3D12Device* device, D3D12_COMMAND_LIST_TYPE type)
    {
        yaget::render::ComPtr<ID3D12Device4> device4;
        HRESULT hr = device->QueryInterface<ID3D12Device4>(&device4);
        yaget::error_handlers::ThrowOnError(hr, "Could not create ID3D12Device4 interface");
        
        yaget::render::ComPtr<ID3D12GraphicsCommandList2> commandList;
        hr = device4->CreateCommandList1(0, type, D3D12_COMMAND_LIST_FLAG_NONE, IID_PPV_ARGS(&commandList));
        yaget::error_handlers::ThrowOnError(hr, "Could not create DX12 Command List");

        YAGET_RENDER_SET_DEBUG_NAME(commandList.Get(), "Yaget Command List");

        return commandList;
    }

} // namespace


//-------------------------------------------------------------------------------------------------
yaget::render::platform::SwapChain::SwapChain(app::WindowFrame windowFrame, const yaget::render::info::Adapter& adapterInfo, ID3D12Device* device, IDXGIFactory* factory, ID3D12CommandQueue* commandQueue)
    : mWindowFrame{ std::move(windowFrame) }
    , mNumBackBuffers{ mWindowFrame.GetSurface().NumBackBuffers() }
    , mTearingSupported{ CheckTearingSupport(factory) }
    , mDevice{ device }
    , mSwapChain{ CreateSwapChain(mWindowFrame, adapterInfo, factory, commandQueue, mNumBackBuffers, mTearingSupported) }
    , mCurrentBackBufferIndex{ mSwapChain->GetCurrentBackBufferIndex() }
    , mRTVDescriptorHeap{ CreateDescriptorHeap(mDevice, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, mNumBackBuffers) }
    , mBackBuffers(mNumBackBuffers, nullptr)
{
    UpdateRenderTargetViews();
    Resize();

    YLOG_INFO("DEVI", "Swap Chain created with '%d' Back Buffers, VSync: '%s' and Tearing Supported: '%s'.", mNumBackBuffers, conv::ToBool(mWindowFrame.GetSurface().VSync()).c_str(), conv::ToBool(mTearingSupported).c_str());
}


//-------------------------------------------------------------------------------------------------
yaget::render::platform::SwapChain::~SwapChain() = default;


//-------------------------------------------------------------------------------------------------
void yaget::render::platform::SwapChain::Resize()
{
    for (int i = 0; i < mNumBackBuffers; ++i)
    {
        // Any references to the back buffers must be released
        // before the swap chain can be resized.
        mBackBuffers[i].Reset();
    }

    DXGI_SWAP_CHAIN_DESC swapChainDesc = {};
    HRESULT hr = mSwapChain->GetDesc(&swapChainDesc);
    error_handlers::ThrowOnError(hr, "Could not get DX12 SwapChain Description");

    hr = mSwapChain->ResizeBuffers(0, 0, 0, DXGI_FORMAT_UNKNOWN, swapChainDesc.Flags);
    error_handlers::ThrowOnError(hr, "Could not resize DX12 SwapChain");

    // this can be used to extract actual buffer size (window size)
    DXGI_SWAP_CHAIN_DESC1 chainDesc = {};
    hr = mSwapChain->GetDesc1(&chainDesc);
    error_handlers::ThrowOnError(hr, "Could not get DX12 swap chain description");
    YLOG_DEBUG("DEVI", "SwapChain::Resize: (%dx%d).", chainDesc.Width, chainDesc.Height);

    mCurrentBackBufferIndex = mSwapChain->GetCurrentBackBufferIndex();
    UpdateRenderTargetViews();
}


//-------------------------------------------------------------------------------------------------
ID3D12Resource* yaget::render::platform::SwapChain::GetCurrentRenderTarget() const
{
    return mBackBuffers[mCurrentBackBufferIndex].Get();
}


//-------------------------------------------------------------------------------------------------
ID3D12DescriptorHeap* yaget::render::platform::SwapChain::GetDescriptorHeap() const
{
    return mRTVDescriptorHeap.Get();
}


//-------------------------------------------------------------------------------------------------
void yaget::render::platform::SwapChain::Present(const time::GameClock& /*gameClock*/, metrics::Channel& /*channel*/)
{
    const uint32_t syncInterval = mWindowFrame.GetSurface().VSync() ? 1 : 0;
    const uint32_t presentFlags = mTearingSupported && syncInterval == 0 ? DXGI_PRESENT_ALLOW_TEARING : 0;

    const HRESULT hr = mSwapChain->Present(syncInterval, presentFlags);
    if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET)
    {
        // driver crashed, let's trigger GPU crash dump
    }

    error_handlers::ThrowOnError(hr, "Could not present DX12 Swap Chain");

    mCurrentBackBufferIndex = mSwapChain->GetCurrentBackBufferIndex();
}


//-------------------------------------------------------------------------------------------------
void yaget::render::platform::SwapChain::UpdateRenderTargetViews()
{
    const auto rtvDescriptorSize = mDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(mRTVDescriptorHeap->GetCPUDescriptorHandleForHeapStart());

    for (int i = 0; i < mNumBackBuffers; ++i)
    {
        ComPtr<ID3D12Resource> backBuffer;
        const HRESULT hr = mSwapChain->GetBuffer(i, IID_PPV_ARGS(&backBuffer));
        error_handlers::ThrowOnError(hr, "Could not get DX12 SwapChain Back Buffer");

        YAGET_RENDER_SET_DEBUG_NAME(backBuffer.Get(), fmt::format("Yaget Back Buffer {}", i));

        mDevice->CreateRenderTargetView(backBuffer.Get(), nullptr, rtvHandle);
        mBackBuffers[i] = backBuffer;

        rtvHandle.Offset(rtvDescriptorSize);
    }
}
