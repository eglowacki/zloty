#include "Render/Device.h"
#include "App/Application.h"
#include "App/AppUtilities.h"
#include "Render/Platform/HardwareDevice.h"

// DirectX 12 specific headers.
#include <d3d12.h>
#include <dxgi1_6.h>
#include <d3dcompiler.h>
//#include <DirectXMath.h>
#include "MathFacade.h"

#include <wrl/client.h>

// D3D12 extension library.
#include <d3dx12.h>

namespace
{
    using namespace Microsoft::WRL;

    ComPtr<ID3D12Device2> g_Device;

    const uint8_t g_NumFrames = 3;
    ComPtr<ID3D12Resource> g_BackBuffers[g_NumFrames];
    ComPtr<ID3D12CommandAllocator> g_CommandAllocators[g_NumFrames];
    ComPtr<ID3D12GraphicsCommandList> g_CommandList;
    ComPtr<ID3D12DescriptorHeap> g_RTVDescriptorHeap;
    ComPtr<ID3D12CommandQueue> g_CommandQueue;
    ComPtr<IDXGISwapChain4> g_SwapChain;
    UINT g_RTVDescriptorSize;
    UINT g_CurrentBackBufferIndex;

    bool g_VSync = true;
    bool g_TearingSupported = false;

    ComPtr<ID3D12Fence> g_Fence;
    uint64_t g_FenceValue = 0;
    uint64_t g_FrameFenceValues[g_NumFrames] = {};
    HANDLE g_FenceEvent;

    uint32_t g_ClientWidth = 1280;
    uint32_t g_ClientHeight = 720;


    void EnableDebugLayer()
    {
#ifdef YAGET_DEBUG
        // Always enable the debug layer before doing anything DX12 related
        // so all possible errors generated while creating DX12 objects
        // are caught by the debug layer.
        ComPtr<ID3D12Debug> debugInterface;
        HRESULT hr = D3D12GetDebugInterface(IID_PPV_ARGS(&debugInterface));
        YAGET_UTIL_THROW_ON_RROR(hr, "Could not get DX12 DebugInterface");

        debugInterface->EnableDebugLayer();
#endif
    }

    ComPtr<IDXGIAdapter4> GetAdapter(bool useWarp)
    {
        ComPtr<IDXGIFactory4> dxgiFactory;
        UINT createFactoryFlags = 0;
#ifdef YAGET_DEBUG
        createFactoryFlags = DXGI_CREATE_FACTORY_DEBUG;
#endif

        HRESULT hr = ::CreateDXGIFactory2(createFactoryFlags, IID_PPV_ARGS(&dxgiFactory));
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
                // creating it. The adapter with the largest dedicated video memory is favored.
                const bool onlyHardware = (dxgiAdapterDesc1.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) == 0;
                if (onlyHardware && SUCCEEDED(D3D12CreateDevice(dxgiAdapter1.Get(), D3D_FEATURE_LEVEL_11_0, __uuidof(ID3D12Device), nullptr)) &&
                    dxgiAdapterDesc1.DedicatedVideoMemory > maxDedicatedVideoMemory)
                {
                    maxDedicatedVideoMemory = dxgiAdapterDesc1.DedicatedVideoMemory;
                    hr = dxgiAdapter1.As(&dxgiAdapter4);
                }
            }
        }

        return dxgiAdapter4;
    }

    ComPtr<ID3D12Device2> CreateDevice(ComPtr<IDXGIAdapter4> adapter)
    {
        ComPtr<ID3D12Device2> d3d12Device2;
        HRESULT hr = D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&d3d12Device2));
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
#endif

        return d3d12Device2;
    }

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

    ComPtr<IDXGISwapChain4> CreateSwapChain(HWND hWnd, ComPtr<ID3D12CommandQueue> commandQueue, uint32_t width, uint32_t height, uint32_t bufferCount)
    {
        ComPtr<IDXGISwapChain4> dxgiSwapChain4;
        ComPtr<IDXGIFactory4> dxgiFactory4;
        UINT createFactoryFlags = 0;
#if defined(_DEBUG)
        createFactoryFlags = DXGI_CREATE_FACTORY_DEBUG;
#endif

        HRESULT hr =CreateDXGIFactory2(createFactoryFlags, IID_PPV_ARGS(&dxgiFactory4));
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

    ComPtr<ID3D12DescriptorHeap> CreateDescriptorHeap(ComPtr<ID3D12Device2> device, D3D12_DESCRIPTOR_HEAP_TYPE type, uint32_t numDescriptors)
    {
        ComPtr<ID3D12DescriptorHeap> descriptorHeap;

        D3D12_DESCRIPTOR_HEAP_DESC desc = {};
        desc.NumDescriptors = numDescriptors;
        desc.Type = type;

        HRESULT hr = device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&descriptorHeap));
        YAGET_UTIL_THROW_ON_RROR(hr, "Could not create DX12 Descriptor");

        return descriptorHeap;
    }

    void UpdateRenderTargetViews(ComPtr<ID3D12Device2> device, ComPtr<IDXGISwapChain4> swapChain, ComPtr<ID3D12DescriptorHeap> descriptorHeap)
    {
        const auto rtvDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

        CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(descriptorHeap->GetCPUDescriptorHandleForHeapStart());

        for (int i = 0; i < g_NumFrames; ++i)
        {
            ComPtr<ID3D12Resource> backBuffer;
            HRESULT ht = swapChain->GetBuffer(i, IID_PPV_ARGS(&backBuffer));

            device->CreateRenderTargetView(backBuffer.Get(), nullptr, rtvHandle);

            g_BackBuffers[i] = backBuffer;

            rtvHandle.Offset(rtvDescriptorSize);
        }
    }

    ComPtr<ID3D12CommandAllocator> CreateCommandAllocator(ComPtr<ID3D12Device2> device, D3D12_COMMAND_LIST_TYPE type)
    {
        ComPtr<ID3D12CommandAllocator> commandAllocator;
        HRESULT hr = device->CreateCommandAllocator(type, IID_PPV_ARGS(&commandAllocator));
        YAGET_UTIL_THROW_ON_RROR(hr, "Could not create DX12 Command Allocator");

        return commandAllocator;
    }

    ComPtr<ID3D12GraphicsCommandList> CreateCommandList(ComPtr<ID3D12Device2> device, ComPtr<ID3D12CommandAllocator> commandAllocator, D3D12_COMMAND_LIST_TYPE type)
    {
        ComPtr<ID3D12GraphicsCommandList> commandList;
        HRESULT hr = device->CreateCommandList(0, type, commandAllocator.Get(), nullptr, IID_PPV_ARGS(&commandList));
        YAGET_UTIL_THROW_ON_RROR(hr, "Could not create DX12 Command List");

        hr = commandList->Close();
        YAGET_UTIL_THROW_ON_RROR(hr, "Could not create close DX12 Command Lis");

        return commandList;
    }

    ComPtr<ID3D12Fence> CreateFence(ComPtr<ID3D12Device2> device)
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
            YAGET_UTIL_THROW_ON_RROR(fenceEvent != nullptr, "Could not Set DX12 Event on completion");

            ::WaitForSingleObject(fenceEvent, static_cast<DWORD>(duration.count()));
        }
    }

    void Flush(ComPtr<ID3D12CommandQueue> commandQueue, ComPtr<ID3D12Fence> fence, uint64_t& fenceValue, HANDLE fenceEvent)
    {
        uint64_t fenceValueForSignal = Signal(commandQueue, fence, fenceValue);
        WaitForFenceValue(fence, fenceValueForSignal, fenceEvent);
    }

    void Render()
    {
        auto commandAllocator = g_CommandAllocators[g_CurrentBackBufferIndex];
        auto backBuffer = g_BackBuffers[g_CurrentBackBufferIndex];

        commandAllocator->Reset();
        g_CommandList->Reset(commandAllocator.Get(), nullptr);
        // Clear the render target.
        {
            CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(backBuffer.Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);

            g_CommandList->ResourceBarrier(1, &barrier);
            FLOAT clearColor[] = { 0.4f, 0.6f, 0.9f, 1.0f };
            CD3DX12_CPU_DESCRIPTOR_HANDLE rtv(g_RTVDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), g_CurrentBackBufferIndex, g_RTVDescriptorSize);

            g_CommandList->ClearRenderTargetView(rtv, clearColor, 0, nullptr);
        }
        // Present
        {
            CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(backBuffer.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
            g_CommandList->ResourceBarrier(1, &barrier);
            HRESULT hr = g_CommandList->Close();
            YAGET_UTIL_THROW_ON_RROR(hr, "Could not close DX12Command List");

            ID3D12CommandList* const commandLists[] = 
            {
                g_CommandList.Get()
            };

            g_CommandQueue->ExecuteCommandLists(_countof(commandLists), commandLists);
            UINT syncInterval = g_VSync ? 1 : 0;
            UINT presentFlags = g_TearingSupported && !g_VSync ? DXGI_PRESENT_ALLOW_TEARING : 0;
            hr = g_SwapChain->Present(syncInterval, presentFlags);
            YAGET_UTIL_THROW_ON_RROR(hr, "Could not Present DX12 SwapChain");

            g_FrameFenceValues[g_CurrentBackBufferIndex] = Signal(g_CommandQueue, g_Fence, g_FenceValue);
            g_CurrentBackBufferIndex = g_SwapChain->GetCurrentBackBufferIndex();

            WaitForFenceValue(g_Fence, g_FrameFenceValues[g_CurrentBackBufferIndex], g_FenceEvent);
        }
    }

}


//-------------------------------------------------------------------------------------------------
yaget::render::Device::Device(Application& app)
    : mApplication(app)
    , mHardwareDevice(new platform::HardwareDevice(app))
{
    //EnableDebugLayer();

    //auto adapter = GetAdapter(false);
    //g_Device = CreateDevice(adapter);

    //g_CommandQueue = CreateCommandQueue(g_Device, D3D12_COMMAND_LIST_TYPE_DIRECT);

   // g_SwapChain = CreateSwapChain(app.GetSurface().Handle<HWND>(), g_CommandQueue, g_ClientWidth, g_ClientHeight, g_NumFrames);

    //g_CurrentBackBufferIndex = g_SwapChain->GetCurrentBackBufferIndex();

    //g_RTVDescriptorHeap = CreateDescriptorHeap(g_Device, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, g_NumFrames);
    //g_RTVDescriptorSize = g_Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

    //UpdateRenderTargetViews(g_Device, g_SwapChain, g_RTVDescriptorHeap);

    //for (int i = 0; i < g_NumFrames; ++i)
    //{
    //    g_CommandAllocators[i] = CreateCommandAllocator(g_Device, D3D12_COMMAND_LIST_TYPE_DIRECT);
    //}

    //g_CommandList = CreateCommandList(g_Device, g_CommandAllocators[g_CurrentBackBufferIndex], D3D12_COMMAND_LIST_TYPE_DIRECT);

    //g_Fence = CreateFence(g_Device);
    //g_FenceEvent = CreateEventHandle();
}


yaget::render::Device::~Device()
{
    //// Make sure the command queue has finished all commands before closing.
    //Flush(g_CommandQueue, g_Fence, g_FenceValue, g_FenceEvent);

    //::CloseHandle(g_FenceEvent);
}

//-------------------------------------------------------------------------------------------------
void yaget::render::Device::RenderFrame(const time::GameClock& /*gameClock*/, metrics::Channel& /*channel*/)
{
    mHardwareDevice->Render();

    mWaiter.Wait();
}


//-------------------------------------------------------------------------------------------------
void yaget::render::Device::Resize()
{
    WaiterScoper scoper(mWaiter);

    mHardwareDevice->Resize();
}


//-------------------------------------------------------------------------------------------------
void yaget::render::Device::SurfaceStateChange()
{
    Resize();
}


//-------------------------------------------------------------------------------------------------
void yaget::render::Device::Waiter::Wait()
{
    if (mPauseCounter == true)
    {
        YLOG_NOTICE("DEVI", "Waiter - We are requested to pause. Stopping.");
        mWaitForRenderThread.notify_one();
        std::unique_lock<std::mutex> locker(mPauseRenderMutex);
        mRenderPaused.wait(locker);
        YLOG_NOTICE("DEVI", "Waiter - Resuming Render.");
    }
}


//-------------------------------------------------------------------------------------------------
void yaget::render::Device::Waiter::BeginPause()
{
    // TODO Look at re-entrent lock (from the same thread)
    // rather then home grown
    if (mUsageCounter++)
    {
        return;
    }

    // We should use Concurency (perf) locker to keep track in RAD
    YLOG_NOTICE("DEVI", "Waiter - Requesting Render pause...");
    std::unique_lock<std::mutex> locker(mPauseRenderMutex);
    mPauseCounter = true;
    mWaitForRenderThread.wait(locker);
    YLOG_NOTICE("DEVI", "Waiter - Render is Paused (resizing commences...)");
}


//-------------------------------------------------------------------------------------------------
void yaget::render::Device::Waiter::EndPause()
{
    if (--mUsageCounter)
    {
        return;
    }

    YLOG_NOTICE("DEVI", "Waiter - Render can start (resizing done)");
    mPauseCounter = false;
    mRenderPaused.notify_one();
}
