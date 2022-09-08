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
    Microsoft::WRL::ComPtr<IDXGISwapChain4> CreateSwapChain(const yaget::app::WindowFrame& windowFrame, const yaget::render::info::Adapter& adapterInfo, IDXGIFactory* factory, ID3D12CommandQueue* commandQueue, uint32_t numBackBuffers, bool tearingSupported)
    {
        const auto& adapterResolution = *adapterInfo.mOutputs.begin()->mResolutions.begin();

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

        Microsoft::WRL::ComPtr<IDXGIFactory> baseFactory(factory);
        Microsoft::WRL::ComPtr<IDXGIFactory2> factory2;
        HRESULT hr = baseFactory.As(&factory2);
        YAGET_UTIL_THROW_ON_RROR(hr, "Could not get factory 2 interface");

        Microsoft::WRL::ComPtr<IDXGISwapChain1> swapChain;
        hr = factory2->CreateSwapChainForHwnd(commandQueue, windowFrame.GetSurface().Handle<HWND>(), &swapChainDesc, nullptr, nullptr, &swapChain);
        YAGET_UTIL_THROW_ON_RROR(hr, "Could not create DX12 SwapChain");

        // Disable the Alt+Enter fullscreen toggle feature. Switching to fullscreen will be handled manually.
        hr = factory2->MakeWindowAssociation(windowFrame.GetSurface().Handle<HWND>(), DXGI_MWA_NO_ALT_ENTER);
        YAGET_UTIL_THROW_ON_RROR(hr, "Could not make DX12 Window Association");

        Microsoft::WRL::ComPtr<IDXGISwapChain4> swapChain4;
        hr = swapChain.As(&swapChain4);
        YAGET_UTIL_THROW_ON_RROR(hr, "Could not get DX12 SwapChain4 interface");

        return swapChain4;
    }

    //-------------------------------------------------------------------------------------------------
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> CreateDescriptorHeap(ID3D12Device* device, D3D12_DESCRIPTOR_HEAP_TYPE type, uint32_t numDescriptors)
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
    Microsoft::WRL::ComPtr<ID3D12CommandAllocator> CreateCommandAllocator(ID3D12Device* device, D3D12_COMMAND_LIST_TYPE type)
    {
        Microsoft::WRL::ComPtr<ID3D12CommandAllocator> commandAllocator;
        HRESULT hr = device->CreateCommandAllocator(type, IID_PPV_ARGS(&commandAllocator));
        YAGET_UTIL_THROW_ON_RROR(hr, "Could not create DX12 Command Allocator");

        YAGET_RENDER_SET_DEBUG_NAME(commandAllocator.Get(), "Yaget Command Allocator");

        return commandAllocator;
    }

    //-------------------------------------------------------------------------------------------------
    Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2> CreateCommandList(ID3D12Device* device, D3D12_COMMAND_LIST_TYPE type)
    {
        Microsoft::WRL::ComPtr<ID3D12Device4> device4;
        HRESULT hr = device->QueryInterface<ID3D12Device4>(&device4);
        YAGET_UTIL_THROW_ON_RROR(hr, "Could not create ID3D12Device4 interface");
        
        Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2> commandList;
        hr = device4->CreateCommandList1(0, type, D3D12_COMMAND_LIST_FLAG_NONE, IID_PPV_ARGS(&commandList));
        YAGET_UTIL_THROW_ON_RROR(hr, "Could not create DX12 Command List");

        YAGET_RENDER_SET_DEBUG_NAME(commandList.Get(), "Yaget Command List");

        return commandList;
    }

} // namespace


//-------------------------------------------------------------------------------------------------
yaget::render::platform::SwapChain::SwapChain(app::WindowFrame windowFrame, const yaget::render::info::Adapter& adapterInfo, ID3D12Device* device, IDXGIFactory* factory)
    : mWindowFrame{ std::move(windowFrame) }
    , mNumBackBuffers{ mWindowFrame.GetSurface().NumBackBuffers() }
    , mTearingSupported{ CheckTearingSupport(factory) }
    , mDevice{ device }
    , mCommandQueue{ std::make_unique<platform::CommandQueue>(mDevice, platform::CommandQueue::Type::Direct) }
    , mSwapChain{ CreateSwapChain(mWindowFrame, adapterInfo, factory, mCommandQueue->GetCommandQueue().Get(), mNumBackBuffers, mTearingSupported) }
    , mCurrentBackBufferIndex{ mSwapChain->GetCurrentBackBufferIndex() }
    , mRTVDescriptorHeap{ CreateDescriptorHeap(mDevice, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, mNumBackBuffers) }
    , mCommander{ mDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV), mRTVDescriptorHeap.Get() }
    , mBackBuffers(mNumBackBuffers, nullptr)
    , mCommandAllocators(mNumBackBuffers, nullptr)
    , mFrameFenceValues(mNumBackBuffers, 0)
{
    UpdateRenderTargetViews();

    for (int i = 0; i < mNumBackBuffers; ++i)
    {
        auto commandAllocator = CreateCommandAllocator(mDevice, D3D12_COMMAND_LIST_TYPE_DIRECT);

        YAGET_RENDER_SET_DEBUG_NAME(commandAllocator.Get(), fmt::format("Yaget Command Allocator: {}", i));

        mCommandAllocators[i] = commandAllocator;
    }

    mCommandList = CreateCommandList(mDevice, D3D12_COMMAND_LIST_TYPE_DIRECT);

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

    //const auto [width, height] = mWindowFrame.GetSurface().GetSize<uint32_t>();

    //hr = mSwapChain->ResizeBuffers(mNumBackBuffers, width, height, swapChainDesc.BufferDesc.Format, swapChainDesc.Flags);
    hr = mSwapChain->ResizeBuffers(0, 0, 0, DXGI_FORMAT_UNKNOWN, swapChainDesc.Flags);
    YAGET_UTIL_THROW_ON_RROR(hr, "Could not resize DX12 SwapChain");

    // this can be used to extract actual buffer size (window size)
    DXGI_SWAP_CHAIN_DESC1 chainDesc = {};
    hr = mSwapChain->GetDesc1(&chainDesc);
    YAGET_UTIL_THROW_ON_RROR(hr, "Could not get DX12 swap chain description");
    YLOG_DEBUG("DEVI", "SwapChain::Resize: (%dx%d).", chainDesc.Width, chainDesc.Height);

    mCurrentBackBufferIndex = mSwapChain->GetCurrentBackBufferIndex();
    UpdateRenderTargetViews();
}


//-------------------------------------------------------------------------------------------------
void yaget::render::platform::SwapChain::Render(const std::vector<Polygon*>& polygons, const time::GameClock& gameClock, metrics::Channel& /*channel*/)
{
    PIXScopedEvent(PIX_COLOR(0, 0, 255), "Frame");

    auto commandAllocator = mCommandAllocators[mCurrentBackBufferIndex];
    const auto renderTarget = mBackBuffers[mCurrentBackBufferIndex].Get();

    HRESULT hr = commandAllocator->Reset();
    YAGET_UTIL_THROW_ON_RROR(hr, "Could not reset DX12 Command Allocator");
    mCommandList->Reset(commandAllocator.Get(), nullptr);

    // Transition to render target so we can start drawing commands to it
    CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(renderTarget, D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
    mCommandList->ResourceBarrier(1, &barrier);

    // this should contain our drawing code here...
    auto setup = [this, &gameClock](auto commandList)
    {
        PrepareCommandList(gameClock, commandList, false);
    };

    const bool oneList = false;
    std::vector<ID3D12GraphicsCommandList*> accumulatedCommandLists;
    accumulatedCommandLists.push_back(mCommandList.Get());
    PrepareCommandList(gameClock, accumulatedCommandLists.back(), true);

    for (const auto& polygon : polygons)
    {
        if (oneList)
        {
            polygon->Render(mCommandList.Get(), setup);
        }
        else
        {
            auto commandLine = polygon->Render(nullptr, setup);
            accumulatedCommandLists.push_back(commandLine);
        }
    }

    for (const auto& commandList : accumulatedCommandLists)
    {
        if (commandList == accumulatedCommandLists.back())
        {
            CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(renderTarget, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
            commandList->ResourceBarrier(1, &barrier);
        }

        hr = commandList->Close();
        YAGET_UTIL_THROW_ON_RROR(hr, "Could not close command list for polygon");
    }

    // Present
    {
        const bool singleExecution = true;

        if (singleExecution)
        {
            const uint32_t numLists = static_cast<uint32_t>(accumulatedCommandLists.size());
            mCommandQueue->GetCommandQueue()->ExecuteCommandLists(numLists, (ID3D12CommandList* const *)accumulatedCommandLists.data());
        }
        else
        {
            for (const auto& commandList : accumulatedCommandLists)
            {
                ID3D12CommandList* commands[] = { commandList };
                mCommandQueue->GetCommandQueue()->ExecuteCommandLists(1, commands);
            }
        }

        const uint32_t syncInterval = mWindowFrame.GetSurface().VSync() ? 1 : 0;
        const uint32_t presentFlags = mTearingSupported && syncInterval == 0 ? DXGI_PRESENT_ALLOW_TEARING : 0;

        hr = mSwapChain->Present(syncInterval, presentFlags);
        if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET)
        {
            // driver crashed, let's trigger GPU crash dump
        }

        YAGET_UTIL_THROW_ON_RROR(hr, "Could not present DX12 Swap Chain");

        mFrameFenceValues[mCurrentBackBufferIndex] = mCommandQueue->Signal();
        mCommandQueue->WaitForFenceValue(mFrameFenceValues[mCurrentBackBufferIndex]);

        mCurrentBackBufferIndex = mSwapChain->GetCurrentBackBufferIndex();
    }

    int z = 0;
    z;
}


//-------------------------------------------------------------------------------------------------
ID3D12CommandAllocator* yaget::render::platform::SwapChain::GetActiveAllocator() const
{
    return mCommandAllocators[mCurrentBackBufferIndex].Get();
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

        YAGET_RENDER_SET_DEBUG_NAME(backBuffer.Get(), fmt::format("Yaget Back Buffer {}", i));

        mDevice->CreateRenderTargetView(backBuffer.Get(), nullptr, rtvHandle);
        mBackBuffers[i] = backBuffer;

        rtvHandle.Offset(rtvDescriptorSize);
    }
}

void yaget::render::platform::SwapChain::PrepareCommandList(const time::GameClock& gameClock, ID3D12GraphicsCommandList* commandList, bool clearRenderTarget)
{
    mCommander.SetRenderTarget(mSwapChain.Get(), commandList, mCurrentBackBufferIndex);

    if (clearRenderTarget)
    {
        const math3d::Color clearColor = math3d::Color::Lerp({ 0.4f, 0.6f, 0.9f }, { 0.6f, 0.9f, 0.4f }, mCurrentColorT);
        mCurrentColorT += (gameClock.GetDeltaTimeSecond() * mColorTDirection) * 0.75f;
        if (mCurrentColorT > 1.0f)
        {
            mColorTDirection = -1.0f;
        }
        else if (mCurrentColorT < 0.0f)
        {
            mColorTDirection = 1.0f;
        }

        mCommander.ClearRenderTarget(clearColor, commandList, mCurrentBackBufferIndex);
    }
}
