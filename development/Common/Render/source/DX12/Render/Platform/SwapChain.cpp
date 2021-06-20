#include "Render/Platform/SwapChain.h"
#include "App/AppUtilities.h"
#include "App/Application.h"

#include <d3d12.h>
#include <dxgi1_6.h>
#include <d3dx12.h>


#if 0

if (mSwapchain != nullptr)
{
    mSwapchain->ResizeBuffers(backbufferCount, mWidth, mHeight, DXGI_FORMAT_R8G8B8A8_UNORM, 0);
}
else
{
    DXGI_SWAP_CHAIN_DESC1 swapchainDesc = {};
    swapchainDesc.BufferCount = backbufferCount;
    swapchainDesc.Width = width;
    swapchainDesc.Height = height;
    swapchainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapchainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapchainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    swapchainDesc.SampleDesc.Count = 1;

    IDXGISwapChain1* swapchain = xgfx::createSwapchain(mWindow, mFactory, mCommandQueue, &swapchainDesc);
    HRESULT swapchainSupport = swapchain->QueryInterface(__uuidof(IDXGISwapChain3), (void**)&swapchain);
    if (SUCCEEDED(swapchainSupport))
    {
        mSwapchain = (IDXGISwapChain3*)swapchain;
    }
}
mFrameIndex = mSwapchain->GetCurrentBackBufferIndex();






inline IDXGISwapChain1* createSwapchain(xwin::Window * window, IDXGIFactory4 * factory,
    ID3D12CommandQueue * queue,
    DXGI_SWAP_CHAIN_DESC1 * swapchainDesc,
    DXGI_SWAP_CHAIN_FULLSCREEN_DESC * fullscreenDesc = nullptr,
    IDXGIOutput * output = nullptr)
{
    xwin::WindowDelegate& del = window->getDelegate();

    IDXGISwapChain1* swapchain = nullptr;
    HRESULT hr = factory->CreateSwapChainForHwnd(queue, del.hwnd, swapchainDesc, fullscreenDesc, output, &swapchain);

    if (!FAILED(hr))
    {
        return swapchain;
    }

    return nullptr;
}

namespace
{
    using namespace Microsoft::WRL;

    //-------------------------------------------------------------------------------------------------
    bool CheckTearingSupport()
    {
        BOOL allowTearing = FALSE;

        // Rather than create the DXGI 1.5 factory interface directly, we create the
        // DXGI 1.4 interface and query for the 1.5 interface. This is to enable the 
        // graphics debugging tools which will not support the 1.5 factory interface 
        // until a future update.
        ComPtr<IDXGIFactory4> factory4;
        if (SUCCEEDED(CreateDXGIFactory1(IID_PPV_ARGS(&factory4))))
        {
            ComPtr<IDXGIFactory5> factory5;
            if (SUCCEEDED(factory4.As(&factory5)))
            {
                if (FAILED(factory5->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING, &allowTearing, sizeof(allowTearing))))
                {
                    allowTearing = FALSE;
                }
            }
        }

        return allowTearing == TRUE;
    }

    //-------------------------------------------------------------------------------------------------
    ComPtr<ID3D12CommandQueue> CreateCommandQueue(ID3D12Device2* device)
    {
        ComPtr<ID3D12CommandQueue> d3d12CommandQueue;

        D3D12_COMMAND_QUEUE_DESC desc = {};
        desc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
        desc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
        desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
        desc.NodeMask = 0;

        HRESULT hr = device->CreateCommandQueue(&desc, IID_PPV_ARGS(&d3d12CommandQueue));
        YAGET_UTIL_THROW_ON_RROR(hr, "Could not create DX12 Command Queue");

        return d3d12CommandQueue;
    }

    //-------------------------------------------------------------------------------------------------
    ComPtr<IDXGISwapChain4> CreateSwapChain(const yaget::app::DisplaySurface& surface, ComPtr<ID3D12CommandQueue> commandQueue, uint32_t bufferCount)
    {
        const auto& size = surface.Size();
        const auto width = static_cast<uint32_t>(size.x);
        const auto height = static_cast<uint32_t>(size.y);
        const auto hWnd = surface.Handle<HWND>();

        ComPtr<IDXGISwapChain4> dxgiSwapChain4;
        ComPtr<IDXGIFactory4> dxgiFactory4;
        UINT createFactoryFlags = 0;
#if defined(_DEBUG)
        createFactoryFlags = DXGI_CREATE_FACTORY_DEBUG;
#endif

        HRESULT hr = CreateDXGIFactory2(createFactoryFlags, IID_PPV_ARGS(&dxgiFactory4));
        YAGET_UTIL_THROW_ON_RROR(hr, "Could not create DXGI Factory2");

        DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
        swapChainDesc.Width = width;
        swapChainDesc.Height = height;
        swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        swapChainDesc.Stereo = FALSE;
        swapChainDesc.SampleDesc = { 1, 0 };
        swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        swapChainDesc.BufferCount = bufferCount;
        swapChainDesc.Scaling = DXGI_SCALING_STRETCH;
        swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
        swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
        // It is recommended to always allow tearing if tearing support is available.
        swapChainDesc.Flags = CheckTearingSupport() ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0;

        ComPtr<IDXGISwapChain1> swapChain1;
        hr = dxgiFactory4->CreateSwapChainForHwnd(
            commandQueue.Get(),
            hWnd,
            &swapChainDesc,
            nullptr,
            nullptr,
            &swapChain1);
        YAGET_UTIL_THROW_ON_RROR(hr, "Could not create DX12 Swap Chain");

        // Disable the Alt+Enter fullscreen toggle feature. Switching to fullscreen
        // will be handled manually.
        hr = dxgiFactory4->MakeWindowAssociation(hWnd, DXGI_MWA_NO_ALT_ENTER);
        YAGET_UTIL_THROW_ON_RROR(hr, "Could not make window association");

        hr = swapChain1.As(&dxgiSwapChain4);
        YAGET_UTIL_THROW_ON_RROR(hr, "Could not get DX12 SwapChain4 from SwapChain1");

        return dxgiSwapChain4;
    }

    //-------------------------------------------------------------------------------------------------
    ComPtr<ID3D12DescriptorHeap> CreateDescriptorHeap(ID3D12Device2* device, D3D12_DESCRIPTOR_HEAP_TYPE type, uint32_t numDescriptors)
    {
        ComPtr<ID3D12DescriptorHeap> descriptorHeap;

        D3D12_DESCRIPTOR_HEAP_DESC desc = {};
        desc.NumDescriptors = numDescriptors;
        desc.Type = type;

        HRESULT hr = device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&descriptorHeap));
        YAGET_UTIL_THROW_ON_RROR(hr, "Could not create DX12 Descriptor");

        return descriptorHeap;
    }

    ComPtr<ID3D12CommandAllocator> CreateCommandAllocator(ID3D12Device2* device, D3D12_COMMAND_LIST_TYPE type)
    {
        ComPtr<ID3D12CommandAllocator> commandAllocator;
        HRESULT hr = device->CreateCommandAllocator(type, IID_PPV_ARGS(&commandAllocator));
        YAGET_UTIL_THROW_ON_RROR(hr, "Could not create DX12 Command Allocator");

        return commandAllocator;
    }

    ComPtr<ID3D12GraphicsCommandList> CreateCommandList(ID3D12Device2* device, ComPtr<ID3D12CommandAllocator> commandAllocator, D3D12_COMMAND_LIST_TYPE type)
    {
        ComPtr<ID3D12GraphicsCommandList> commandList;
        HRESULT hr = device->CreateCommandList(0, type, commandAllocator.Get(), nullptr, IID_PPV_ARGS(&commandList));
        YAGET_UTIL_THROW_ON_RROR(hr, "Could not create DX12 Command List");

        hr = commandList->Close();
        YAGET_UTIL_THROW_ON_RROR(hr, "Could not create close DX12 Command Lis");

        return commandList;
    }

    ComPtr<ID3D12Fence> CreateFence(ID3D12Device2* device)
    {
        ComPtr<ID3D12Fence> fence;

        HRESULT hr = device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence));
        YAGET_UTIL_THROW_ON_RROR(hr, "Could not create DX12 Fence");

        return fence;
    }

    HANDLE CreateEventHandle()
    {
        HANDLE fenceEvent = ::CreateEvent(NULL, FALSE, FALSE, NULL);
        YAGET_UTIL_THROW_ON_RROR(fenceEvent != nullptr, "Could not create DX12 Fence Event");

        return fenceEvent;
    }

    uint64_t Signal(ComPtr<ID3D12CommandQueue> commandQueue, ComPtr<ID3D12Fence> fence, uint64_t& fenceValue)
    {
        uint64_t fenceValueForSignal = ++fenceValue;
        HRESULT hr = commandQueue->Signal(fence.Get(), fenceValueForSignal);
        YAGET_UTIL_THROW_ON_RROR(hr, "Could not create DX12 Fence Event");

        return fenceValueForSignal;
    }

    void WaitForFenceValue(ComPtr<ID3D12Fence> fence, uint64_t fenceValue, HANDLE fenceEvent, std::chrono::milliseconds duration = std::chrono::milliseconds::max())
    {
        if (fence->GetCompletedValue() < fenceValue)
        {
            HRESULT hr = fence->SetEventOnCompletion(fenceValue, fenceEvent);
            YAGET_UTIL_THROW_ON_RROR(hr, "Could not Set DX12 Event on completion");

            ::WaitForSingleObject(fenceEvent, static_cast<DWORD>(duration.count()));
        }
    }

    void Flush(ComPtr<ID3D12CommandQueue> commandQueue, ComPtr<ID3D12Fence> fence, uint64_t& fenceValue, HANDLE fenceEvent)
    {
        uint64_t fenceValueForSignal = Signal(commandQueue, fence, fenceValue);
        WaitForFenceValue(fence, fenceValueForSignal, fenceEvent);
    }

}


//-------------------------------------------------------------------------------------------------
yaget::render::platform::SwapChain::SwapChain(Application& app, ID3D12Device2* device, uint32_t numFrames)
    : mApplication(app)
    , mDevice(device)
    , mCommandQueue(CreateCommandQueue(mDevice))
    , mSwapChain(CreateSwapChain(mApplication.GetSurface(), mCommandQueue, 3))
    , mCurrentBackBufferIndex(mSwapChain->GetCurrentBackBufferIndex())
    , mDescriptorHeap(CreateDescriptorHeap(mDevice, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, numFrames))
    , mFence(CreateFence(mDevice))
    , mFenceEvent(CreateEventHandle())
    , mDescriptorSize(mDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV))
    , mNumFrames(numFrames)
    , mBackBuffers(mNumFrames)
    , mAllocators(mNumFrames)
    , mFrameFenceValues(mNumFrames)
{
    UpdateRenderTargetViews(mDevice);
    for (uint32_t i = 0; i < mNumFrames; ++i)
    {
        mAllocators[i] = CreateCommandAllocator(mDevice, D3D12_COMMAND_LIST_TYPE_DIRECT);
    }

    mCommandList = CreateCommandList(mDevice, mAllocators[mCurrentBackBufferIndex], D3D12_COMMAND_LIST_TYPE_DIRECT);

    Resize();
}


//-------------------------------------------------------------------------------------------------
yaget::render::platform::SwapChain::~SwapChain()
{
    Flush(mCommandQueue, mFence, mFenceValue, mFenceEvent);

    ::CloseHandle(mFenceEvent);
}


void yaget::render::platform::SwapChain::Render(const time::GameClock& /*gameClock*/, metrics::Channel& /*channel*/)
{
    auto commandAllocator = mAllocators[mCurrentBackBufferIndex];
    auto backBuffer = mBackBuffers[mCurrentBackBufferIndex];

    commandAllocator->Reset();
    mCommandList->Reset(commandAllocator.Get(), nullptr);
    // Clear the render target.
    {
        CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(backBuffer.Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);

        mCommandList->ResourceBarrier(1, &barrier);
        FLOAT clearColor[] = { 0.4f, 0.6f, 0.9f, 1.0f };
        CD3DX12_CPU_DESCRIPTOR_HANDLE rtv(mDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), mCurrentBackBufferIndex, mDescriptorSize);

        mCommandList->ClearRenderTargetView(rtv, clearColor, 0, nullptr);
    }

    // Present
    {
        CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(backBuffer.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
        mCommandList->ResourceBarrier(1, &barrier);
        HRESULT hr = mCommandList->Close();
        YAGET_UTIL_THROW_ON_RROR(hr, "Could not close DX12Command List");

        ID3D12CommandList* const commandLists[] =
        {
            mCommandList.Get()
        };

        mCommandQueue->ExecuteCommandLists(_countof(commandLists), commandLists);
        const UINT syncInterval = true ? 1 : 0;
        const UINT presentFlags = 0;
        //UINT presentFlags = g_TearingSupported && !g_VSync ? DXGI_PRESENT_ALLOW_TEARING : 0;
        hr = mSwapChain->Present(syncInterval, presentFlags);
        if (FAILED(hr))
        {
            Resize();
            //hr = mDevice->GetDeviceRemovedReason();
            //YAGET_UTIL_THROW_ON_RROR(hr, "Could not Present DX12 SwapChain");
        }

        mFrameFenceValues[mCurrentBackBufferIndex] = Signal(mCommandQueue, mFence, mFenceValue);
        mCurrentBackBufferIndex = mSwapChain->GetCurrentBackBufferIndex();

        WaitForFenceValue(mFence, mFrameFenceValues[mCurrentBackBufferIndex], mFenceEvent);
    }
}

//-------------------------------------------------------------------------------------------------
void yaget::render::platform::SwapChain::Resize()
{
    // Signal and increment the fence value.
    const UINT64 fence = mFenceValue;
    HRESULT hr = mCommandQueue->Signal(mFence.Get(), fence);
    mFenceValue++;

    // Wait until the previous frame is finished.
    if (mFence->GetCompletedValue() < fence)
    {
        hr = mFence->SetEventOnCompletion(fence, mFenceEvent);
        WaitForSingleObjectEx(mFenceEvent, INFINITE, false);
    }

    const auto& size = mApplication.GetSurface().Size();
    const auto width = static_cast<uint32_t>(size.x);
    const auto height = static_cast<uint32_t>(size.y);

    // Flush the GPU queue to make sure the swap chain's back buffers
    // are not being referenced by an in-flight command list.
    Flush(mCommandQueue, mFence, mFenceValue, mFenceEvent);

    for (uint32_t i = 0; i < mNumFrames; ++i)
    {
        // Any references to the back buffers must be released
        // before the swap chain can be resized.
        mBackBuffers[i].Reset();
        mFrameFenceValues[i] = mFrameFenceValues[mCurrentBackBufferIndex];
    }

    DXGI_SWAP_CHAIN_DESC swapChainDesc = {};
    hr = mSwapChain->GetDesc(&swapChainDesc);
    YAGET_UTIL_THROW_ON_RROR(hr, "Could not get SwapChain description");

    hr = mSwapChain->ResizeBuffers(0, 0, 0, DXGI_FORMAT_UNKNOWN, swapChainDesc.Flags);
    YAGET_UTIL_THROW_ON_RROR(hr, "Could not resize buffers");

    mCurrentBackBufferIndex = mSwapChain->GetCurrentBackBufferIndex();

    UpdateRenderTargetViews(mDevice);
}


//-------------------------------------------------------------------------------------------------
void yaget::render::platform::SwapChain::UpdateRenderTargetViews(ID3D12Device2* device)
{
    const auto rtvDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(mDescriptorHeap->GetCPUDescriptorHandleForHeapStart());

    for (uint32_t i = 0; i < mNumFrames; ++i)
    {
        ComPtr<ID3D12Resource> backBuffer;
        HRESULT hr = mSwapChain->GetBuffer(i, IID_PPV_ARGS(&backBuffer));

        device->CreateRenderTargetView(backBuffer.Get(), nullptr, rtvHandle);

        mBackBuffers[i] = backBuffer;

        rtvHandle.Offset(rtvDescriptorSize);
    }
}
#endif