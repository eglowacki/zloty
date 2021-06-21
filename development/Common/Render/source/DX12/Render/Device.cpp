#include "Render/Device.h"
#include "App/Application.h"
#include "App/AppUtilities.h"
#include "Metrics/Concurrency.h"
#include "Platform/Adapter.h"
#include "Platform/Support.h"
#include "Render/Platform/SwapChain.h"
#include "StringHelpers.h"
#include "Time/GameClock.h"

#include <dxgi1_4.h>



namespace
{
    //-------------------------------------------------------------------------------------------------
    std::vector<char> ReadFile(const std::string& filename)
    {
        std::ifstream file(filename, std::ios::ate | std::ios::binary);
        bool exists = (bool)file;

        if (!exists || !file.is_open())
        {
            throw std::runtime_error("failed to open file!");
        }

        size_t fileSize = (size_t)file.tellg();
        std::vector<char> buffer(fileSize);

        file.seekg(0);
        file.read(buffer.data(), fileSize);

        return buffer;
    };

    //-------------------------------------------------------------------------------------------------
    struct Vertex
    {
        DirectX::XMFLOAT3 position;
        DirectX::XMFLOAT4 color;
    };

}


//-------------------------------------------------------------------------------------------------
yaget::render::Device::Device(Application& app)
    : mApplication(app)
    , mAdapter(std::make_unique<platform::Adapter>())
    , mCommandQueue(std::make_unique<platform::CommandQueue>(mAdapter->GetDevice()))
    , mFence(std::make_unique<platform::Fence>(mAdapter->GetDevice()))
    , mSwapChain(std::make_unique<platform::SwapChain>(app, mAdapter->GetFactory(), mCommandQueue->Get(), FrameCount))
    , mFrameIndex(mSwapChain->Get()->GetCurrentBackBufferIndex())
{
    // Create descriptor heaps.
    {
        // Describe and create a render target view (RTV) descriptor heap.
        D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
        rtvHeapDesc.NumDescriptors = FrameCount;
        rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
        rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
        HRESULT hr = mAdapter->GetDevice()->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&mRTVHeap));
        YAGET_UTIL_THROW_ON_RROR(hr, "Could not create DX12 Descriptor Heap");

        mRTVDescriptorSize = mAdapter->GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    }

    // Create frame resources.
    {
        CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(mRTVHeap->GetCPUDescriptorHandleForHeapStart());

        // Create a RTV for each frame.
        for (UINT n = 0; n < FrameCount; n++)
        {
            HRESULT hr = mSwapChain->Get()->GetBuffer(n, IID_PPV_ARGS(&mRenderTargets[n]));
            YAGET_UTIL_THROW_ON_RROR(hr, "Could not get DX12 Render Target");

            mAdapter->GetDevice()->CreateRenderTargetView(mRenderTargets[n].Get(), nullptr, rtvHandle);
            rtvHandle.Offset(1, mRTVDescriptorSize);
        }
    }

    HRESULT hr = mAdapter->GetDevice()->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&mCommandAllocator));
    YAGET_UTIL_THROW_ON_RROR(hr, "Could not create DX12 Command Allocator");

    // Create an empty root signature.
    {
        CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
        rootSignatureDesc.Init(0, nullptr, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

        ComPtr<ID3DBlob> signature;
        ComPtr<ID3DBlob> errors;
        hr = D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &errors);
        if (FAILED(hr))
        {
            const char* errStr = errors ? static_cast<const char*>(errors->GetBufferPointer()) : nullptr;

            const auto message = fmt::format("Could not Serialize DX12 RootSignature{}", errStr ? std::string(". Additional error message : '") + errStr + "'" : "");
            YAGET_UTIL_THROW_ON_RROR(hr, message.c_str());
        }

        hr = mAdapter->GetDevice()->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&mRootSignature));
    }

    // Create the pipeline state, which includes compiling and loading shaders.
    {
        const std::string vertCompiledPath = yaget::util::ExpendEnv("$(AppFolder)/shaders.vert.cso", nullptr);
        const std::string fragCompiledPath = yaget::util::ExpendEnv("$(AppFolder)/shaders.frag.cso", nullptr);

        std::vector<char> vsBytecodeData = ReadFile(vertCompiledPath);
        std::vector<char> fsBytecodeData = ReadFile(fragCompiledPath);

        // Define the vertex input layout.
        D3D12_INPUT_ELEMENT_DESC inputElementDescs[] =
        {
            { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
        };

        D3D12_SHADER_BYTECODE vsBytecode{};
        D3D12_SHADER_BYTECODE psBytecode{};

        vsBytecode.pShaderBytecode = vsBytecodeData.data();
        vsBytecode.BytecodeLength = vsBytecodeData.size();

        psBytecode.pShaderBytecode = fsBytecodeData.data();
        psBytecode.BytecodeLength = fsBytecodeData.size();


        // Describe and create the graphics pipeline state object (PSO).
        D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
        psoDesc.InputLayout = { inputElementDescs, _countof(inputElementDescs) };
        psoDesc.pRootSignature = mRootSignature.Get();
        psoDesc.VS = vsBytecode;
        psoDesc.PS = psBytecode;
        psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
        psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
        psoDesc.DepthStencilState.DepthEnable = FALSE;
        psoDesc.DepthStencilState.StencilEnable = FALSE;
        psoDesc.SampleMask = UINT_MAX;
        psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        psoDesc.NumRenderTargets = 1;
        psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
        psoDesc.SampleDesc.Count = 1;
        hr = mAdapter->GetDevice()->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&mPipelineState));
        YAGET_UTIL_THROW_ON_RROR(hr, "Could not create DX12 Graphics Pipeline State");
    }

    hr = mAdapter->GetDevice()->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, mCommandAllocator.Get(), mPipelineState.Get(), IID_PPV_ARGS(&mCommandList));
    YAGET_UTIL_THROW_ON_RROR(hr, "Could not create DX12 Command List");

    hr = mCommandList->Close();
    YAGET_UTIL_THROW_ON_RROR(hr, "Could not close DX12 Command List");

    // Create the vertex buffer.
    {
        const auto& surface = mApplication.GetSurface();
        const auto& size = surface.Size();
        const auto width = static_cast<uint32_t>(size.x);
        const auto height = static_cast<uint32_t>(size.y);
        const float aspectRatio = size.x / size.y;

        // Define the geometry for a triangle.
        Vertex triangleVertices[] =
        {
            { { 0.0f, 0.25f * aspectRatio, 0.0f }, { 1.0f, 0.0f, 0.0f, 1.0f } },
            { { 0.25f, -0.25f * aspectRatio, 0.0f }, { 0.0f, 1.0f, 0.0f, 1.0f } },
            { { -0.25f, -0.25f * aspectRatio, 0.0f }, { 0.0f, 0.0f, 1.0f, 1.0f } }
        };

        const UINT vertexBufferSize = sizeof(triangleVertices);

        // Note: using upload heaps to transfer static data like vert buffers is not 
        // recommended. Every time the GPU needs it, the upload heap will be marshalled 
        // over. Please read up on Default Heap usage. An upload heap is used here for 
        // code simplicity and because there are very few verts to actually transfer.
        //
        CD3DX12_HEAP_PROPERTIES heapProperties{ D3D12_HEAP_TYPE_UPLOAD };
        CD3DX12_RESOURCE_DESC bufferDescription = CD3DX12_RESOURCE_DESC::Buffer(vertexBufferSize);

        hr = mAdapter->GetDevice()->CreateCommittedResource(
            &heapProperties,
            D3D12_HEAP_FLAG_NONE,
            &bufferDescription,
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_PPV_ARGS(&mVertexBuffer));
        YAGET_UTIL_THROW_ON_RROR(hr, "Could not create DX12 Committed Resource for Vertex Buffer");

        // Copy the triangle data to the vertex buffer.
        UINT8* pVertexDataBegin;
        CD3DX12_RANGE readRange(0, 0);		// We do not intend to read from this resource on the CPU.
        hr = mVertexBuffer->Map(0, &readRange, reinterpret_cast<void**>(&pVertexDataBegin));
        YAGET_UTIL_THROW_ON_RROR(hr, "Could not map DX12 Vertex Buffer");

        memcpy(pVertexDataBegin, triangleVertices, sizeof(triangleVertices));
        mVertexBuffer->Unmap(0, nullptr);

        // Initialize the vertex buffer view.
        mVertexBufferView.BufferLocation = mVertexBuffer->GetGPUVirtualAddress();
        mVertexBufferView.StrideInBytes = sizeof(Vertex);
        mVertexBufferView.SizeInBytes = vertexBufferSize;
    }

    mFence->Wait(*mCommandQueue);
    mFrameIndex = mSwapChain->Get()->GetCurrentBackBufferIndex();

    const auto& surface = mApplication.GetSurface();
    const auto& size = surface.Size();
    const auto width = static_cast<uint32_t>(size.x);
    const auto height = static_cast<uint32_t>(size.y);

    mViewport = CD3DX12_VIEWPORT(0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height));
    mScissorRect = CD3DX12_RECT(0, 0, static_cast<LONG>(width), static_cast<LONG>(height));
}


//-------------------------------------------------------------------------------------------------
yaget::render::Device::~Device()
{
    mFence->Wait(*mCommandQueue);
}


//-------------------------------------------------------------------------------------------------
void yaget::render::Device::Resize()
{
    WaiterScoper scoper(mWaiter);
    mFence->Wait(*mCommandQueue);
}


//-------------------------------------------------------------------------------------------------
void yaget::render::Device::SurfaceStateChange()
{
}


//-------------------------------------------------------------------------------------------------
void yaget::render::Device::RenderFrame(const time::GameClock& /*gameClock*/, metrics::Channel& /*channel*/)
{
    metrics::Channel channel("RenderFrame", YAGET_METRICS_CHANNEL_FILE_LINE);

    // populate command list
    //============================================================================
    // Command list allocators can only be reset when the associated 
    // command lists have finished execution on the GPU; apps should use 
    // fences to determine GPU execution progress.
    HRESULT hr = mCommandAllocator->Reset();
    YAGET_UTIL_THROW_ON_RROR(hr, "Could not reset DX12 Command Allocator");

    // However, when ExecuteCommandList() is called on a particular command 
    // list, that command list can then be reset at any time and must be before 
    // re-recording.
    hr = mCommandList->Reset(mCommandAllocator.Get(), mPipelineState.Get());
    YAGET_UTIL_THROW_ON_RROR(hr, "Could not reset DX12 Command List with CommandAllocator and PipelineState");

    // Set necessary state.
    mCommandList->SetGraphicsRootSignature(mRootSignature.Get());
    mCommandList->RSSetViewports(1, &mViewport);
    mCommandList->RSSetScissorRects(1, &mScissorRect);

    // Indicate that the back buffer will be used as a render target.
    CD3DX12_RESOURCE_BARRIER resourceBarrier = CD3DX12_RESOURCE_BARRIER::Transition(mRenderTargets[mFrameIndex].Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
    mCommandList->ResourceBarrier(1, &resourceBarrier);

    CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(mRTVHeap->GetCPUDescriptorHandleForHeapStart(), mFrameIndex, mRTVDescriptorSize);
    mCommandList->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);

    // Record commands.
    const float clearColor[] = { 0.0f, 0.2f, 0.4f, 1.0f };
    mCommandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
    mCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    mCommandList->IASetVertexBuffers(0, 1, &mVertexBufferView);
    mCommandList->DrawInstanced(3, 1, 0, 0);

    // Indicate that the back buffer will now be used to present.
    CD3DX12_RESOURCE_BARRIER  resourceBarrier1 = CD3DX12_RESOURCE_BARRIER::Transition(mRenderTargets[mFrameIndex].Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
    mCommandList->ResourceBarrier(1, &resourceBarrier1);

    hr = mCommandList->Close();
    YAGET_UTIL_THROW_ON_RROR(hr, "Could not close DX12 Command List");
    //============================================================================

    // Execute the command list.
    ID3D12CommandList* ppCommandLists[] = { mCommandList.Get() };
    mCommandQueue->Get()->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

    // Present the frame.
    hr = mSwapChain->Get()->Present(1, 0);
    YAGET_UTIL_THROW_ON_RROR(hr, "Could not present DX12 Swap Chain");

    mFence->Wait(*mCommandQueue);
    mFrameIndex = mSwapChain->Get()->GetCurrentBackBufferIndex();

    mWaiter.Wait();
    yaget::platform::Sleep(8, time::kMilisecondUnit);
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
