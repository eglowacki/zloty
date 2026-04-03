#include "D3D12MemAlloc.h"

#include "Render/Platform/SwapChain.h"
#include "StringHelpers.h"
#include "Render/Platform/DeviceDebugger.h"
#include "Render/AdapterInfo.h"
#include "App/Application.h"
#include "MathFacade.h"
#include "Core/ErrorHandlers.h"
#include "Render/Pipeline/RenderShaders.h"

#include <d3dx12.h>
#include <dxgi1_6.h>

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


    yaget::render::ComPtr<ID3D12Resource> CreateDepthStencilBuffer(ID3D12Device* device, ID3D12DescriptorHeap* dsDescriptorHeap, size_t width, size_t height, int depthStencilFlags)
    {
        if (depthStencilFlags == 0)
        {
            return {};
        }

        DXGI_FORMAT depthStencilFormat = DXGI_FORMAT_D32_FLOAT;
        if (depthStencilFlags == (yaget::render::platform::SwapChain::DepthBufferFlag | yaget::render::platform::SwapChain::StencilBufferFlag))
        {
            depthStencilFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
        }

        YAGET_ASSERT(depthStencilFlags & yaget::render::platform::SwapChain::DepthBufferFlag, "Can not create Stencil Buffer without Depth Buffer specified.");

        D3D12_CLEAR_VALUE depthOptimizedClearValue = {};
        depthOptimizedClearValue.Format = depthStencilFormat;//DXGI_FORMAT_D24_UNORM_S8_UINT;//DXGI_FORMAT_D32_FLOAT;
        depthOptimizedClearValue.DepthStencil.Depth = 1.0f;
        depthOptimizedClearValue.DepthStencil.Stencil = 0;

        D3D12_RESOURCE_DESC depthStencilDesc = {};
        depthStencilDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        depthStencilDesc.Alignment = 0;
        depthStencilDesc.Width = width;
        depthStencilDesc.Height = static_cast<uint32_t>(height);
        depthStencilDesc.DepthOrArraySize = 1;
        depthStencilDesc.MipLevels = 0;
        depthStencilDesc.Format = depthStencilFormat;//DXGI_FORMAT_D24_UNORM_S8_UINT;//DXGI_FORMAT_D32_FLOAT;
        depthStencilDesc.SampleDesc.Count = 1;
        depthStencilDesc.SampleDesc.Quality = 0;
        depthStencilDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
        depthStencilDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

        D3D12_HEAP_PROPERTIES heapProperties =
        {
            .Type = D3D12_HEAP_TYPE_DEFAULT,
            .CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
            .MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN,
            .CreationNodeMask = 1,
            .VisibleNodeMask = 1
        };

        yaget::render::ComPtr<ID3D12Resource> depthStencilBuffer;
        HRESULT hr = device->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &depthStencilDesc, D3D12_RESOURCE_STATE_DEPTH_WRITE, &depthOptimizedClearValue, IID_PPV_ARGS(&depthStencilBuffer));
        yaget::error_handlers::ThrowOnError(hr, "Could not create DX12 Depth Stencil buffer");

        YAGET_RENDER_SET_DEBUG_NAME(depthStencilBuffer.Get(), "Depth Stencil Buffer");

        D3D12_DEPTH_STENCIL_VIEW_DESC depthStencilViewDesc = {};
        depthStencilViewDesc.Format = depthStencilFormat;//DXGI_FORMAT_D24_UNORM_S8_UINT;//DXGI_FORMAT_D32_FLOAT;
        depthStencilViewDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
        depthStencilViewDesc.Flags = D3D12_DSV_FLAG_NONE;

        device->CreateDepthStencilView(depthStencilBuffer.Get(), &depthStencilViewDesc, dsDescriptorHeap->GetCPUDescriptorHandleForHeapStart());
        
        return depthStencilBuffer;
    }


} // namespace


int yaget::render::platform::SwapChain::DepthBufferFlag = 1 << 0;
int yaget::render::platform::SwapChain::StencilBufferFlag = 1 << 1;

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
    , mDepthStencilFlags{ DepthBufferFlag | StencilBufferFlag }
    , mDSVDescriptorHeap{ mDepthStencilFlags ? CreateDescriptorHeap(mDevice, D3D12_DESCRIPTOR_HEAP_TYPE_DSV, 1) : ComPtr<ID3D12DescriptorHeap>{} }
    , mDepthStencilBuffer{ CreateDepthStencilBuffer(mDevice, mDSVDescriptorHeap.Get(), adapterInfo.GetSelectedResolution().mWidth, adapterInfo.GetSelectedResolution().mHeight, mDepthStencilFlags) }
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
    mDepthStencilBuffer.Reset();
    mDSVDescriptorHeap.Reset();

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

    mDSVDescriptorHeap = CreateDescriptorHeap(mDevice, D3D12_DESCRIPTOR_HEAP_TYPE_DSV, 1);
    mDepthStencilBuffer = CreateDepthStencilBuffer(mDevice, mDSVDescriptorHeap.Get(), chainDesc.Width, chainDesc.Height, mDepthStencilFlags);
}


//-------------------------------------------------------------------------------------------------
ID3D12Resource* yaget::render::platform::SwapChain::GetCurrentRenderTarget() const
{
    return mBackBuffers[mCurrentBackBufferIndex].Get();
}


//-------------------------------------------------------------------------------------------------
ID3D12DescriptorHeap* yaget::render::platform::SwapChain::GetRTVDescriptorHeap() const
{
    return mRTVDescriptorHeap.Get();
}


//-------------------------------------------------------------------------------------------------
ID3D12DescriptorHeap* yaget::render::platform::SwapChain::GetDSVDescriptorHeap() const
{
    return mDSVDescriptorHeap.Get();
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

        YAGET_RENDER_SET_DEBUG_NAME(backBuffer.Get(), std::format("Yaget Back Buffer {}", i));

        mDevice->CreateRenderTargetView(backBuffer.Get(), nullptr, rtvHandle);
        mBackBuffers[i] = backBuffer;

        rtvHandle.Offset(rtvDescriptorSize);
    }
}
