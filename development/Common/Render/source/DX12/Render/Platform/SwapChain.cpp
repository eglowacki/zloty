#include "DeviceDebugger.h"
#include "Render/Platform/SwapChain.h"
#include "App/AppUtilities.h"
#include "App/Application.h"
#include "CommandQueue.h"
#include "StringHelpers.h"
#include "Math/YagetMath.h"

#include <d3d12.h>
#include <dxgi1_6.h>
#include <d3dx12.h>

namespace 
{
    //-------------------------------------------------------------------------------------------------
    bool CheckTearingSupport(const Microsoft::WRL::ComPtr<IDXGIFactory4>& factory)
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
    Microsoft::WRL::ComPtr<IDXGISwapChain4> CreateSwapChain(const yaget::app::WindowFrame& windowFrame, IDXGIFactory4* factory, ID3D12CommandQueue* commandQueue, uint32_t numBackBuffers, bool tearingSupported)
    {
        const auto [width, height] = windowFrame.GetSurface().GetSize<uint32_t>();

        DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
        swapChainDesc.BufferCount = numBackBuffers;
        swapChainDesc.Width = width;
        swapChainDesc.Height = height;
        swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
        swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
        swapChainDesc.SampleDesc = { 1, 0 };
        swapChainDesc.Flags = tearingSupported ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0;

        Microsoft::WRL::ComPtr<IDXGISwapChain1> swapChain;
        HRESULT hr = factory->CreateSwapChainForHwnd(commandQueue, windowFrame.GetSurface().Handle<HWND>(), &swapChainDesc, nullptr, nullptr, &swapChain);
        YAGET_UTIL_THROW_ON_RROR(hr, "Could not create DX12 SwapChain");

        // Disable the Alt+Enter fullscreen toggle feature. Switching to fullscreen will be handled manually.
        hr = factory->MakeWindowAssociation(windowFrame.GetSurface().Handle<HWND>(), DXGI_MWA_NO_ALT_ENTER);
        YAGET_UTIL_THROW_ON_RROR(hr, "Could not make DX12 Window Association");

        Microsoft::WRL::ComPtr<IDXGISwapChain4> swapChain4;
        hr = swapChain.As(&swapChain4);
        YAGET_UTIL_THROW_ON_RROR(hr, "Could not get DX12 SwapChain4 interface");

        return swapChain4;
    }

    //-------------------------------------------------------------------------------------------------
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> CreateDescriptorHeap(ID3D12Device4* device, D3D12_DESCRIPTOR_HEAP_TYPE type, uint32_t numDescriptors)
    {
        D3D12_DESCRIPTOR_HEAP_DESC desc = {};
        desc.NumDescriptors = numDescriptors;
        desc.Type = type;
    
        Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> descriptorHeap;
        HRESULT hr = device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&descriptorHeap));
        YAGET_UTIL_THROW_ON_RROR(hr, "Could not create DX12 DescriptorHeap");

        YAGET_RENDER_SET_DEBUG_NAME(descriptorHeap.Get(), "Yaget Descriptor Heap");

        return descriptorHeap;
    }

    //-------------------------------------------------------------------------------------------------
    Microsoft::WRL::ComPtr<ID3D12CommandAllocator> CreateCommandAllocator(ID3D12Device2* device, D3D12_COMMAND_LIST_TYPE type)
    {
        Microsoft::WRL::ComPtr<ID3D12CommandAllocator> commandAllocator;
        HRESULT hr = device->CreateCommandAllocator(type, IID_PPV_ARGS(&commandAllocator));
        YAGET_UTIL_THROW_ON_RROR(hr, "Could not create DX12 Command Allocator");

        return commandAllocator;
    }

    //-------------------------------------------------------------------------------------------------
    Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2> CreateCommandList(ID3D12Device2* device, ID3D12CommandAllocator* commandAllocator, D3D12_COMMAND_LIST_TYPE type)
    {
        Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList;
        HRESULT hr = device->CreateCommandList(0, type, commandAllocator, nullptr, IID_PPV_ARGS(&commandList));
        YAGET_UTIL_THROW_ON_RROR(hr, "Could not create DX12 Command List");

        hr = commandList->Close();
        YAGET_UTIL_THROW_ON_RROR(hr, "Could not close DX12 Command List");

        Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2> commandList2;
        hr = commandList.As(&commandList2);
        YAGET_UTIL_THROW_ON_RROR(hr, "Could not get DX12 Command List2 interface");

        return commandList2;
    }

} // namespace


//-------------------------------------------------------------------------------------------------
yaget::render::platform::SwapChain::SwapChain(app::WindowFrame windowFrame, const ComPtr<ID3D12Device4>& device, IDXGIFactory4* factory)
    : mWindowFrame{ std::move(windowFrame) }
    , mNumBackBuffers{ mWindowFrame.GetSurface().NumBackBuffers() }
    , mTearingSupported{ CheckTearingSupport(factory) }
    , mDevice{ device }
    , mCommandQueue{ std::make_unique<platform::CommandQueue>(mDevice.Get(), platform::CommandQueue::Type::Direct) }
    , mSwapChain{ CreateSwapChain(mWindowFrame, factory, mCommandQueue->GetCommandQueue().Get(), mNumBackBuffers, mTearingSupported) }
    , mCurrentBackBufferIndex{ mSwapChain->GetCurrentBackBufferIndex() }
    , mRTVDescriptorHeap{ CreateDescriptorHeap(mDevice.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_RTV, mNumBackBuffers) }
    , mRTVDescriptorSize{ mDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV) }
    , mBackBuffers(mNumBackBuffers, nullptr)
    , mCommandAllocators(mNumBackBuffers, nullptr)
    , mFrameFenceValues(mNumBackBuffers, 0)
{
    UpdateRenderTargetViews();

    for (int i = 0; i < mNumBackBuffers; ++i)
    {
        auto commandAllocator = CreateCommandAllocator(mDevice.Get(), D3D12_COMMAND_LIST_TYPE_DIRECT);

        YAGET_RENDER_SET_DEBUG_NAME(commandAllocator.Get(), fmt::format("Yaget Command Allocator: {}", i));

        mCommandAllocators[i] = commandAllocator;
    }

    mCommandList = CreateCommandList(mDevice.Get(), mCommandAllocators[mCurrentBackBufferIndex].Get(), D3D12_COMMAND_LIST_TYPE_DIRECT);

    Resize();

    YLOG_INFO("DEVI", "Swap Chain created with '%d' Back Buffers, VSync: '%s' and Tearing Supported: '%s'.", mNumBackBuffers, conv::ToBool(mWindowFrame.GetSurface().VSync()).c_str(), conv::ToBool(mTearingSupported).c_str());
}


//-------------------------------------------------------------------------------------------------
yaget::render::platform::SwapChain::~SwapChain()
{
    mCommandQueue->Flush();
}


//-------------------------------------------------------------------------------------------------
void yaget::render::platform::SwapChain::Resize()
{
    // Flush the GPU queue to make sure the swap chain's back buffers
    // are not being referenced by an in-flight command list.
    mCommandQueue->Flush();

    for (int i = 0; i < mNumBackBuffers; ++i)
    {
        // Any references to the back buffers must be released
        // before the swap chain can be resized.
        mBackBuffers[i].Reset();
        mFrameFenceValues[i] = mFrameFenceValues[mCurrentBackBufferIndex];
    }

    DXGI_SWAP_CHAIN_DESC swapChainDesc = {};
    HRESULT hr = mSwapChain->GetDesc(&swapChainDesc);
    YAGET_UTIL_THROW_ON_RROR(hr, "Could not get DX12 SwapChain Description");

    const auto [width, height] = mWindowFrame.GetSurface().GetSize<uint32_t>();

    YLOG_DEBUG("DEVI", "SwapChain::Resize: (%dx%d).", width, height);

    hr = mSwapChain->ResizeBuffers(mNumBackBuffers, width, height, swapChainDesc.BufferDesc.Format, swapChainDesc.Flags);
    YAGET_UTIL_THROW_ON_RROR(hr, "Could not resize DX12 SwapChain");

    mCurrentBackBufferIndex = mSwapChain->GetCurrentBackBufferIndex();
    UpdateRenderTargetViews();
}


//-------------------------------------------------------------------------------------------------
void yaget::render::platform::SwapChain::Render(const time::GameClock& gameClock, metrics::Channel& /*channel*/)
{
    auto commandAllocator = mCommandAllocators[mCurrentBackBufferIndex];
    const auto backBuffer = mBackBuffers[mCurrentBackBufferIndex];

    HRESULT hr = commandAllocator->Reset();
    YAGET_UTIL_THROW_ON_RROR(hr, "Could not reset DX12 Command Allocator");

    mCommandList->Reset(commandAllocator.Get(), nullptr);

    // Clear the render target.
    {
        CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(backBuffer.Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);

        mCommandList->ResourceBarrier(1, &barrier);

        const math3d::Color currentClearColor = math3d::Color::Lerp({ 0.4f, 0.6f, 0.9f }, { 0.6f, 0.9f, 0.4f }, mCurrentColorT);
        mCurrentColorT += (gameClock.GetDeltaTimeSecond() * mColorTDirection) * 0.75f;
        if (mCurrentColorT > 1.0f)
        {
            mColorTDirection = -1.0f;
        }
        else if (mCurrentColorT < 0.0f)
        {
            mColorTDirection = 1.0f;
        }

        const CD3DX12_CPU_DESCRIPTOR_HANDLE rtv(mRTVDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), mCurrentBackBufferIndex, mRTVDescriptorSize);

        mCommandList->ClearRenderTargetView(rtv, currentClearColor, 0, nullptr);
    }

    // Present
    {
        CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(backBuffer.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
        mCommandList->ResourceBarrier(1, &barrier);

        hr = mCommandList->Close();
        YAGET_UTIL_THROW_ON_RROR(hr, "Could not close DX12 Command List");

        ID3D12CommandList* const commandLists[] = { mCommandList.Get() };
        mCommandQueue->GetCommandQueue()->ExecuteCommandLists(_countof(commandLists), commandLists);

        mFrameFenceValues[mCurrentBackBufferIndex] = mCommandQueue->Signal();

        const uint32_t syncInterval = mWindowFrame.GetSurface().VSync() ? 1 : 0;
        const uint32_t presentFlags = mTearingSupported && syncInterval == 0 ? DXGI_PRESENT_ALLOW_TEARING : 0;

        hr = mSwapChain->Present(syncInterval, presentFlags);
        YAGET_UTIL_THROW_ON_RROR(hr, "Could not present DX12 Swap Chain");

        mCurrentBackBufferIndex = mSwapChain->GetCurrentBackBufferIndex();

        mCommandQueue->WaitForFenceValue(mFrameFenceValues[mCurrentBackBufferIndex]);
    }
}


//-------------------------------------------------------------------------------------------------
void yaget::render::platform::SwapChain::UpdateRenderTargetViews()
{
    const auto rtvDescriptorSize = mDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(mRTVDescriptorHeap->GetCPUDescriptorHandleForHeapStart());

    for (int i = 0; i < mNumBackBuffers; ++i)
    {
        ComPtr<ID3D12Resource> backBuffer;
        HRESULT hr = mSwapChain->GetBuffer(i, IID_PPV_ARGS(&backBuffer));
        YAGET_UTIL_THROW_ON_RROR(hr, "Could not get DX12 SwapChain Back Buffer");

        YAGET_RENDER_SET_DEBUG_NAME(backBuffer.Get(), fmt::format("Yaget Back Buffer: {}", i));

        mDevice->CreateRenderTargetView(backBuffer.Get(), nullptr, rtvHandle);
        mBackBuffers[i] = backBuffer;

        rtvHandle.Offset(rtvDescriptorSize);
    }
}
