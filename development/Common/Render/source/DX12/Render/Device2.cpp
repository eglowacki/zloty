#include "Render/Device.h"
#include "App/Application.h"
#include "App/AppUtilities.h"
#include "MathFacade.h"
#include "Platform/Adapter.h"
#include "Platform/DeviceDebugger.h"
#include "Platform/Support.h"
#include "StringHelpers.h"

// DirectX 12 specific headers.
#include "HashUtilities.h"
#include <d3d12.h>
#include <d3dcompiler.h>
#include <d3dx12.h>
#include <dxgi1_4.h>
#include <wrl/client.h>
#include <fstream>


namespace
{
    using namespace Microsoft::WRL;

    const uint32_t MaxRenderBuffers = 2;

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
    ComPtr<ID3D12CommandQueue> CreateCommandQueue(const ComPtr<ID3D12Device>& device)
    {
        D3D12_COMMAND_QUEUE_DESC queueDesc = {};
        queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
        queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

        ComPtr<ID3D12CommandQueue> commandQueue;

        const HRESULT hr = device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&commandQueue));
        yaget::error_handlers::ThrowOnError(hr, "Could not create DX12 Command Queue");

        return commandQueue;
    }


    //-------------------------------------------------------------------------------------------------
    ComPtr<ID3D12CommandAllocator> CreateCommandAllocator(const ComPtr<ID3D12Device>& device)
    {
        ComPtr<ID3D12CommandAllocator> commandAllocator;

        const HRESULT hr = device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&commandAllocator));
        yaget::error_handlers::ThrowOnError(hr, "Could not create DX12 Command Allocator");

        return commandAllocator;
    }


    //-------------------------------------------------------------------------------------------------
    ComPtr<ID3D12Fence> CreateFence(const ComPtr<ID3D12Device>& device)
    {
        ComPtr<ID3D12Fence> fence;

        const HRESULT hr = device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence));
        yaget::error_handlers::ThrowOnError(hr, "Could not create DX12 Fence");

        return fence;
    }


    //-------------------------------------------------------------------------------------------------
    HANDLE CreateFenceEvent()
    {
        HANDLE fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
        yaget::error_handlers::ThrowOnError(fenceEvent, "Could not DX12 Fence Event");

        return fenceEvent;
    }


    //-------------------------------------------------------------------------------------------------
        // Create the root signature.
    ComPtr<ID3D12RootSignature> CreateRootSignature(const ComPtr<ID3D12Device>& device)
    {
        D3D12_FEATURE_DATA_ROOT_SIGNATURE featureData = {};

        // This is the highest version the sample supports. If
        // CheckFeatureSupport succeeds, the HighestVersion returned will not be
        // greater than this.
        featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;

        if (FAILED(device->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &featureData, sizeof(featureData))))
        {
            featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
        }

        D3D12_DESCRIPTOR_RANGE1 ranges[1];
        ranges[0].BaseShaderRegister = 0;
        ranges[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
        ranges[0].NumDescriptors = 1;
        ranges[0].RegisterSpace = 0;
        ranges[0].OffsetInDescriptorsFromTableStart = 0;
        ranges[0].Flags = D3D12_DESCRIPTOR_RANGE_FLAG_NONE;

        D3D12_ROOT_PARAMETER1 rootParameters[1];
        rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
        rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;

        rootParameters[0].DescriptorTable.NumDescriptorRanges = 1;
        rootParameters[0].DescriptorTable.pDescriptorRanges = ranges;

        D3D12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc;
        rootSignatureDesc.Version = D3D_ROOT_SIGNATURE_VERSION_1_1;
        rootSignatureDesc.Desc_1_1.Flags =
            D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
        rootSignatureDesc.Desc_1_1.NumParameters = 1;
        rootSignatureDesc.Desc_1_1.pParameters = rootParameters;
        rootSignatureDesc.Desc_1_1.NumStaticSamplers = 0;
        rootSignatureDesc.Desc_1_1.pStaticSamplers = nullptr;

        ComPtr<ID3DBlob> signature;
        ComPtr<ID3DBlob> error;
        ComPtr<ID3D12RootSignature> rootSignature;
        HRESULT hr = D3D12SerializeVersionedRootSignature(&rootSignatureDesc, &signature, &error);
        if (FAILED(hr))
        {
            const char* errStr = static_cast<const char*>(error->GetBufferPointer());

            const auto message = fmt::format("Could not DX12 Serialize Versioned Root Signature. Additional error message: '%s'", errStr);
            error_handlers::ThrowOnError(hr, message.c_str());
        }

        hr = device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&rootSignature));
        error_handlers::ThrowOnError(hr, "Could not create DX12 Root Signature");

        rootSignature->SetName(L"Yaget Root Signature");

        return rootSignature;
    }


    //-------------------------------------------------------------------------------------------------
        // Create the pipeline state, which includes compiling and loading shaders.
    void CreatePipeline(const ComPtr<ID3D12Device>& device, 
        const ComPtr<ID3D12RootSignature>& rootSignature, 
        ComPtr<ID3D12PipelineState>& pipelineState,
        ComPtr<ID3D12Resource>& uniformBuffer, 
        ComPtr<ID3D12DescriptorHeap>& uniformBufferHeap,
        uint8_t*& mappedUniformBuffer,
        yaget::render::Device::UniformData& uboVs)
    {
        ComPtr<ID3DBlob> vertexShader;
        ComPtr<ID3DBlob> pixelShader;

#if YAGET_DEBUG_RENDER == 1
        // Enable better shader debugging with the graphics debugging tools.
        UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
        UINT compileFlags = 0;
#endif

        //std::string path = "";
        //char pBuf[1024] = {};

        ////$(AssetsFolder)/Shaders
        ////'$(AppDataFolder)/Baked/Shaders/
        ////_getcwd(pBuf, 1024);
        //path = pBuf;
        //path += "\\";
        //std::wstring wpath = std::wstring(path.begin(), path.end());

        //std::string vertCompiledPath = path, fragCompiledPath = path;
        //vertCompiledPath += "assets\\triangle.vert.dxbc";
        //fragCompiledPath += "assets\\triangle.frag.dxbc";
        //
        std::string vertCompiledPath = yaget::util::ExpendEnv("$(AppFolder)/triangle.vert.cso", nullptr);
        std::string fragCompiledPath = yaget::util::ExpendEnv("$(AppFolder)/triangle.frag.cso", nullptr);

        HRESULT hr = 0;
//#define COMPILESHADERS
#ifdef COMPILESHADERS
        std::wstring vertPath = wpath + L"assets\\triangle.vert.hlsl";
        std::wstring fragPath = wpath + L"assets\\triangle.frag.hlsl";

        ComPtr<ID3DBlob> errors;
        HRESULT hr = D3DCompileFromFile(vertPath.c_str(), nullptr, nullptr, "main", "vs_5_0", compileFlags, 0, &vertexShader, &errors);
        if (FAILED(hr))
        {
            const char* errStr = errors ? static_cast<const char*>(errors->GetBufferPointer()) : nullptr;

            const auto message = fmt::format("Could not compile DX12 Vertex Shader from file: '{}'{}", yaget::conv::wide_to_utf8(vertPath.c_str()), errStr ? std::string(". Additional error message : '") + errStr + "'" : "");
            error_handlers::ThrowOnError(hr, message.c_str());
        }

        hr = D3DCompileFromFile(fragPath.c_str(), nullptr, nullptr, "main", "ps_5_0", compileFlags, 0, &pixelShader, &errors);
        if (FAILED(hr))
        {
            const char* errStr = static_cast<const char*>(errors->GetBufferPointer());

            const auto message = fmt::format("Could not DX12 Pixel Shader CompileFromFile. Additional error message: '%s'", errStr);
            error_handlers::ThrowOnError(hr, message.c_str());
        }

        std::ofstream vsOut(vertCompiledPath, std::ios::out | std::ios::binary), fsOut(fragCompiledPath, std::ios::out | std::ios::binary);

        vsOut.write((const char*)vertexShader->GetBufferPointer(), vertexShader->GetBufferSize());
        fsOut.write((const char*)pixelShader->GetBufferPointer(), pixelShader->GetBufferSize());

#else
        std::vector<char> vsBytecodeData = ReadFile(vertCompiledPath);
        std::vector<char> fsBytecodeData = ReadFile(fragCompiledPath);

#endif
        // Define the vertex input layout.
        D3D12_INPUT_ELEMENT_DESC inputElementDescs[] = 
        {
            { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            { "COLOR", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
        };

        // Create the UBO.
        {
            // Note: using upload heaps to transfer static data like vert
            // buffers is not recommended. Every time the GPU needs it, the
            // upload heap will be marshalled over. Please read up on Default
            // Heap usage. An upload heap is used here for code simplicity and
            // because there are very few verts to actually transfer.
            D3D12_HEAP_PROPERTIES heapProps;
            heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;
            heapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
            heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
            heapProps.CreationNodeMask = 1;
            heapProps.VisibleNodeMask = 1;

            D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
            heapDesc.NumDescriptors = 1;
            heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
            heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;

            hr = device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&uniformBufferHeap));
            error_handlers::ThrowOnError(hr, "Could not create DX12 Create Descriptor Heap");

            D3D12_RESOURCE_DESC uboResourceDesc;
            uboResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
            uboResourceDesc.Alignment = 0;
            uboResourceDesc.Width = (sizeof(uboVs) + 255) & ~255;
            uboResourceDesc.Height = 1;
            uboResourceDesc.DepthOrArraySize = 1;
            uboResourceDesc.MipLevels = 1;
            uboResourceDesc.Format = DXGI_FORMAT_UNKNOWN;
            uboResourceDesc.SampleDesc.Count = 1;
            uboResourceDesc.SampleDesc.Quality = 0;
            uboResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
            uboResourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

            hr = device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &uboResourceDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&uniformBuffer));
            error_handlers::ThrowOnError(hr, "Could not create DX12 Create Committed Resource");

            uniformBufferHeap->SetName(L"Constant Buffer Upload Resource Heap");

            D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
            cbvDesc.BufferLocation = uniformBuffer->GetGPUVirtualAddress();
            cbvDesc.SizeInBytes = (sizeof(uboVs) + 255) & ~255; // CB size is required to be 256-byte aligned.

            D3D12_CPU_DESCRIPTOR_HANDLE cbvHandle(uniformBufferHeap->GetCPUDescriptorHandleForHeapStart());
            cbvHandle.ptr = cbvHandle.ptr + device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV) * 0;

            device->CreateConstantBufferView(&cbvDesc, cbvHandle);

            // We do not intend to read from this resource on the CPU. (End is
            // less than or equal to begin)
            D3D12_RANGE readRange;
            readRange.Begin = 0;
            readRange.End = 0;

            hr = uniformBuffer->Map(0, &readRange, reinterpret_cast<void**>(&mappedUniformBuffer));
            error_handlers::ThrowOnError(hr, "Could not map DX12 Mapped Uniform Buffer");

            memcpy(mappedUniformBuffer, &uboVs, sizeof(uboVs));
            uniformBuffer->Unmap(0, &readRange);
        }

        // Describe and create the graphics pipeline state object (PSO).
        D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
        psoDesc.InputLayout = { inputElementDescs, _countof(inputElementDescs) };
        psoDesc.pRootSignature = rootSignature.Get();

        D3D12_SHADER_BYTECODE vsBytecode;
        D3D12_SHADER_BYTECODE psBytecode;

#ifdef COMPILESHADERS
        vsBytecode.pShaderBytecode = vertexShader->GetBufferPointer();
        vsBytecode.BytecodeLength = vertexShader->GetBufferSize();

        psBytecode.pShaderBytecode = pixelShader->GetBufferPointer();
        psBytecode.BytecodeLength = pixelShader->GetBufferSize();
#else
        vsBytecode.pShaderBytecode = vsBytecodeData.data();
        vsBytecode.BytecodeLength = vsBytecodeData.size();

        psBytecode.pShaderBytecode = fsBytecodeData.data();
        psBytecode.BytecodeLength = fsBytecodeData.size();
#endif

        psoDesc.VS = vsBytecode;
        psoDesc.PS = psBytecode;

        D3D12_RASTERIZER_DESC rasterDesc;
        rasterDesc.FillMode = D3D12_FILL_MODE_SOLID;
        rasterDesc.CullMode = D3D12_CULL_MODE_NONE;
        rasterDesc.FrontCounterClockwise = FALSE;
        rasterDesc.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
        rasterDesc.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
        rasterDesc.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
        rasterDesc.DepthClipEnable = TRUE;
        rasterDesc.MultisampleEnable = FALSE;
        rasterDesc.AntialiasedLineEnable = FALSE;
        rasterDesc.ForcedSampleCount = 0;
        rasterDesc.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;

        psoDesc.RasterizerState = rasterDesc;

        D3D12_BLEND_DESC blendDesc;
        blendDesc.AlphaToCoverageEnable = FALSE;
        blendDesc.IndependentBlendEnable = FALSE;
        const D3D12_RENDER_TARGET_BLEND_DESC defaultRenderTargetBlendDesc = {
            FALSE,
            FALSE,
            D3D12_BLEND_ONE,
            D3D12_BLEND_ZERO,
            D3D12_BLEND_OP_ADD,
            D3D12_BLEND_ONE,
            D3D12_BLEND_ZERO,
            D3D12_BLEND_OP_ADD,
            D3D12_LOGIC_OP_NOOP,
            D3D12_COLOR_WRITE_ENABLE_ALL,
        };
        for (UINT i = 0; i < D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT; ++i)
            blendDesc.RenderTarget[i] = defaultRenderTargetBlendDesc;

        psoDesc.BlendState = blendDesc;
        psoDesc.DepthStencilState.DepthEnable = FALSE;
        psoDesc.DepthStencilState.StencilEnable = FALSE;
        psoDesc.SampleMask = UINT_MAX;
        psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        psoDesc.NumRenderTargets = 1;
        psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
        psoDesc.SampleDesc.Count = 1;

        hr = device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pipelineState));
        error_handlers::ThrowOnError(hr, "Could not create DX12 Graphics Pipeline State");
    }


#if YAGET_DEBUG_RENDER == 1
    //-------------------------------------------------------------------------------------------------
    ComPtr<ID3D12DebugDevice> CreateDebugDevice(const ComPtr<ID3D12Device>& device)
    {
        ComPtr<ID3D12DebugDevice> debugDevice;
        const HRESULT hr = device->QueryInterface(IID_PPV_ARGS(&debugDevice));
        error_handlers::ThrowOnError(hr, "Could not get DX12 Debug Device");

        return debugDevice;
    }
#endif


    //-------------------------------------------------------------------------------------------------
    void CreateVertexBuffer(const ComPtr<ID3D12Device>& device, const yaget::render::Device::Vertex* vertexBufferData, ComPtr<ID3D12Resource>& vertexBuffer)
    {
        using Vertex = yaget::render::Device::Vertex;

        // Create the vertex buffer.
        const UINT vertexBufferSize = sizeof(Vertex) * 3;

        // Note: using upload heaps to transfer static data like vert buffers is
        // not recommended. Every time the GPU needs it, the upload heap will be
        // marshalled over. Please read up on Default Heap usage. An upload heap
        // is used here for code simplicity and because there are very few verts
        // to actually transfer.
        D3D12_HEAP_PROPERTIES heapProps;
        heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;
        heapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
        heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
        heapProps.CreationNodeMask = 1;
        heapProps.VisibleNodeMask = 1;

        D3D12_RESOURCE_DESC vertexBufferResourceDesc;
        vertexBufferResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        vertexBufferResourceDesc.Alignment = 0;
        vertexBufferResourceDesc.Width = vertexBufferSize;
        vertexBufferResourceDesc.Height = 1;
        vertexBufferResourceDesc.DepthOrArraySize = 1;
        vertexBufferResourceDesc.MipLevels = 1;
        vertexBufferResourceDesc.Format = DXGI_FORMAT_UNKNOWN;
        vertexBufferResourceDesc.SampleDesc.Count = 1;
        vertexBufferResourceDesc.SampleDesc.Quality = 0;
        vertexBufferResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        vertexBufferResourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

        HRESULT hr = device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &vertexBufferResourceDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&vertexBuffer));
        error_handlers::ThrowOnError(hr, "Could not create DX12 Committed Resource for Vertex Buffer");

        // Copy the triangle data to the vertex buffer.
        UINT8* pVertexDataBegin;

        // We do not intend to read from this resource on the CPU.
        D3D12_RANGE readRange;
        readRange.Begin = 0;
        readRange.End = 0;

        hr = vertexBuffer->Map(0, &readRange, reinterpret_cast<void**>(&pVertexDataBegin));
        error_handlers::ThrowOnError(hr, "Could not Map DX12 Vertex Buffer");

        memcpy(pVertexDataBegin, vertexBufferData, vertexBufferSize);
        vertexBuffer->Unmap(0, nullptr);
    }


    //-------------------------------------------------------------------------------------------------
    void CreateIndexBuffer(const ComPtr<ID3D12Device>& device, const uint32_t* indexBufferData, ComPtr<ID3D12Resource>& indexBuffer)
    {
        const UINT indexBufferSize = sizeof(uint32_t) * 3;

        // Note: using upload heaps to transfer static data like vert buffers is
        // not recommended. Every time the GPU needs it, the upload heap will be
        // marshalled over. Please read up on Default Heap usage. An upload heap
        // is used here for code simplicity and because there are very few verts
        // to actually transfer.
        D3D12_HEAP_PROPERTIES heapProps{};
        heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;
        heapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
        heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
        heapProps.CreationNodeMask = 1;
        heapProps.VisibleNodeMask = 1;

        D3D12_RESOURCE_DESC vertexBufferResourceDesc{};
        vertexBufferResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        vertexBufferResourceDesc.Alignment = 0;
        vertexBufferResourceDesc.Width = indexBufferSize;
        vertexBufferResourceDesc.Height = 1;
        vertexBufferResourceDesc.DepthOrArraySize = 1;
        vertexBufferResourceDesc.MipLevels = 1;
        vertexBufferResourceDesc.Format = DXGI_FORMAT_UNKNOWN;
        vertexBufferResourceDesc.SampleDesc.Count = 1;
        vertexBufferResourceDesc.SampleDesc.Quality = 0;
        vertexBufferResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        vertexBufferResourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

        HRESULT hr = device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &vertexBufferResourceDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&indexBuffer));
        error_handlers::ThrowOnError(hr, "Could not create DX12 Committed Resource for Index Buffer");

        // Copy the triangle data to the vertex buffer.
        UINT8* pVertexDataBegin;

        // We do not intend to read from this resource on the CPU.
        D3D12_RANGE readRange;
        readRange.Begin = 0;
        readRange.End = 0;

        hr = indexBuffer->Map(0, &readRange, reinterpret_cast<void**>(&pVertexDataBegin));
        error_handlers::ThrowOnError(hr, "Could not Map DX12 Index Buffer");

        memcpy(pVertexDataBegin, indexBufferData, indexBufferSize);
        indexBuffer->Unmap(0, nullptr);
    }

}


//-------------------------------------------------------------------------------------------------
yaget::render::Device::Device(Application& app)
    : mApplication(app)
      //, mHardwareDevice(new platform::HardwareDevice(app))
#if YAGET_DEBUG_RENDER == 1
      , mDeviceDebugger(std::make_unique<platform::DeviceDebugger>())
#endif
      , mAdapter(std::make_unique<platform::Adapter>())
      , mDevice(mAdapter->GetDevice())
#if YAGET_DEBUG_RENDER == 1
      , mDebugDevice(CreateDebugDevice(mDevice))
#endif
      , mCommandQueue(CreateCommandQueue(mDevice))
      , mCommandAllocator(CreateCommandAllocator(mDevice))
      , mFence(CreateFence(mDevice))
      , mFenceValue(1)
      , mFenceEvent(CreateFenceEvent())
      , mBackBuffers(MaxRenderBuffers)
{
    SetupSwapChain();
    InitBackBuffers();

    InitializeResources();

    CreatePipeline(mDevice, mRootSignature, mPipelineState, mUniformBuffer, mUniformBufferHeap, mMappedUniformBuffer, mUboVs);

    HRESULT hr = mDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, mCommandAllocator.Get(), mPipelineState.Get(), IID_PPV_ARGS(&mCommandList));
    error_handlers::ThrowOnError(hr, "Could not create DX12 Command List");

    mCommandList->SetName(L"Yaget Command List");

    // Command lists are created in the recording state, but there is nothing
    // to record yet. The main loop expects it to be closed, so close it now.
    hr = mCommandList->Close();
    error_handlers::ThrowOnError(hr, "Could not close DX12 Command List");

    CreateVertexBuffer(mDevice, mVertexBufferData, mVertexBuffer);
    // Initialize the vertex buffer view.
    mVertexBufferView.BufferLocation = mVertexBuffer->GetGPUVirtualAddress();
    mVertexBufferView.StrideInBytes = sizeof(Vertex);
    mVertexBufferView.SizeInBytes = sizeof(mVertexBufferData);

    CreateIndexBuffer(mDevice, mIndexBufferData, mIndexBuffer);
    // Initialize the vertex buffer view.
    mIndexBufferView.BufferLocation = mIndexBuffer->GetGPUVirtualAddress();
    mIndexBufferView.Format = DXGI_FORMAT_R32_UINT;
    mIndexBufferView.SizeInBytes = sizeof(mIndexBufferData);

    // Create synchronization objects and wait until assets have been uploaded
    // to the GPU.
    // Wait for the command list to execute; we are reusing the same command
    // list in our main loop but for now, we just want to wait for setup to
    // complete before continuing.
    // Signal and increment the fence value.
    const UINT64 fence = mFenceValue;
    hr = mCommandQueue->Signal(mFence.Get(), fence);
    error_handlers::ThrowOnError(hr, "Could not signal DX12 Command Queue");

    mFenceValue++;

    // Wait until the previous frame is finished.
    if (mFence->GetCompletedValue() < fence)
    {
        hr = mFence->SetEventOnCompletion(fence, mFenceEvent);
        error_handlers::ThrowOnError(hr, "Could not set DX12 Event On Completion");
        WaitForSingleObject(mFenceEvent, INFINITE);
    }

    mFrameIndex = mSwapchain->GetCurrentBackBufferIndex();
}


//-------------------------------------------------------------------------------------------------
yaget::render::Device::~Device()
{
    mSwapchain->SetFullscreenState(false, nullptr);
    mSwapchain = nullptr;

    // destroyCommands ========================================================================
    mCommandList->Reset(mCommandAllocator.Get(), mPipelineState.Get());
    mCommandList->ClearState(mPipelineState.Get());
    HRESULT hr = mCommandList->Close();
    error_handlers::ThrowOnError(hr, "Could not close DX12 Command List");

    ID3D12CommandList* ppCommandLists[] = { mCommandList.Get() };
    mCommandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

    // Wait for GPU to finish work
    const UINT64 fence = mFenceValue;
    hr = mCommandQueue->Signal(mFence.Get(), fence);
    error_handlers::ThrowOnError(hr, "Could not signal DX12 mCommandQueue with mFence");

    mFenceValue++;
    if (mFence->GetCompletedValue() < fence)
    {
        hr = mFence->SetEventOnCompletion(fence, mFenceEvent);
        error_handlers::ThrowOnError(hr, "Could not set DX12 Event On Completion");

        WaitForSingleObject(mFenceEvent, INFINITE);
    }

    mCommandList = nullptr;
    // destroyCommands ========================================================================


    // destroyFrameBuffer =====================================================================
    mBackBuffers.clear();
    mRtvHeap = nullptr;
    // destroyFrameBuffer =====================================================================


    // destroyResources =======================================================================
    CloseHandle(mFenceEvent);

    mPipelineState = nullptr;
    mRootSignature = nullptr;

    //mVertexBuffer = nullptr;
    //mIndexBuffer = nullptr;
    //
    mUniformBuffer = nullptr;
    mUniformBufferHeap = nullptr;
    // destroyResources =======================================================================


    // destroyAPI =============================================================================
    // destroyAPI =============================================================================
    mFence = nullptr;

    hr = mCommandAllocator->Reset();
    error_handlers::ThrowOnError(hr, "Could not reset DX12 mCommandAllocator");
    mCommandAllocator = nullptr;

    mCommandQueue = nullptr;

    mAdapter = nullptr;
    //if (mDevice)
    //{
    //    mDevice->Release();
    //    mDevice = nullptr;
    //}

    //if (mAdapter)
    //{
    //    mAdapter->Release();
    //    mAdapter = nullptr;
    //}

    //if (mFactory)
    //{
    //    mFactory->Release();
    //    mFactory = nullptr;
    //}

//#if defined(_DEBUG)
//    if (mDebugController)
//    {
//        mDebugController->Release();
//        mDebugController = nullptr;
//    }

//    D3D12_RLDO_FLAGS flags =
//        D3D12_RLDO_SUMMARY | D3D12_RLDO_DETAIL | D3D12_RLDO_IGNORE_INTERNAL;
//
//    mDebugDevice->ReportLiveDeviceObjects(flags);
//
//    if (mDebugDevice)
//    {
//        mDebugDevice->Release();
//        mDebugDevice = nullptr;
//    }
//#endif

#if YAGET_DEBUG_RENDER == 1
    mDeviceDebugger = nullptr;

    //const D3D12_RLDO_FLAGS flags = D3D12_RLDO_SUMMARY | D3D12_RLDO_DETAIL | D3D12_RLDO_IGNORE_INTERNAL;
    const D3D12_RLDO_FLAGS flags = D3D12_RLDO_DETAIL | D3D12_RLDO_IGNORE_INTERNAL;

    mDebugDevice->ReportLiveDeviceObjects(flags);
    mDebugDevice = nullptr;
#endif
}


//-------------------------------------------------------------------------------------------------
void yaget::render::Device::RenderFrame(const time::GameClock& gameClock, metrics::Channel& channel)
{
    //mHardwareDevice->Render(gameClock, channel);

    yaget::platform::Sleep(16, time::kMilisecondUnit);
    mWaiter.Wait();
}


//-------------------------------------------------------------------------------------------------
void yaget::render::Device::ResetBackBuffers()
{
    for (size_t i = 0; i < MaxRenderBuffers; ++i)
    {
        if (mBackBuffers[i])
        {
            mBackBuffers[i] = nullptr;
        }
    }

    mRtvHeap = nullptr;
}


//-------------------------------------------------------------------------------------------------
void yaget::render::Device::SetupSwapChain()
{
    const auto& size = mApplication.GetSurface().Size();
    const auto width = static_cast<uint32_t>(size.x);
    const auto height = static_cast<uint32_t>(size.y);


    if (mSwapchain)
    {
        mSwapchain->ResizeBuffers(MaxRenderBuffers, width, height, DXGI_FORMAT_R8G8B8A8_UNORM, 0);
    }
    else
    {
        DXGI_SWAP_CHAIN_DESC1 swapchainDesc = {};
        swapchainDesc.BufferCount = MaxRenderBuffers;
        swapchainDesc.Width = width;
        swapchainDesc.Height = height;
        swapchainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        swapchainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        swapchainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
        swapchainDesc.SampleDesc.Count = 1;

        //IDXGISwapChain1* swapchain = xgfx::createSwapchain(mWindow, mFactory, mCommandQueue, &swapchainDesc);
        DXGI_SWAP_CHAIN_FULLSCREEN_DESC* fullscreenDesc = nullptr;

        ComPtr<IDXGISwapChain1> swapchain;
        HRESULT hr = mAdapter->GetFactory()->CreateSwapChainForHwnd(mCommandQueue.Get(), mApplication.GetSurface().Handle<HWND>(), &swapchainDesc, fullscreenDesc, nullptr, &swapchain);
        error_handlers::ThrowOnError(hr, "Could not get DX12 SwapChain");

        hr = swapchain->QueryInterface(__uuidof(IDXGISwapChain3), &mSwapchain);
        error_handlers::ThrowOnError(hr, "Could not get DX12 SwapChain3 Interface");
    }

    mFrameIndex = mSwapchain->GetCurrentBackBufferIndex();
}


//-------------------------------------------------------------------------------------------------
void yaget::render::Device::InitBackBuffers()
{
    mFrameIndex = mSwapchain->GetCurrentBackBufferIndex();

    // Create descriptor heaps.
    {
        // Describe and create a render target view (RTV) descriptor heap.
        D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
        rtvHeapDesc.NumDescriptors = MaxRenderBuffers;
        rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
        rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
        const HRESULT hr = mDevice->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&mRtvHeap));
        error_handlers::ThrowOnError(hr, "Could not get DX12 DescriptorHeap Interface");

        mRtvDescriptorSize = mDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    }

    // Create frame resources.
    {
        D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle(mRtvHeap->GetCPUDescriptorHandleForHeapStart());

        // Create a RTV for each frame.
        for (UINT n = 0; n < MaxRenderBuffers; ++n)
        {
            HRESULT hr = mSwapchain->GetBuffer(n, IID_PPV_ARGS(&mBackBuffers[n]));
            error_handlers::ThrowOnError(hr, "Could not get DX12 BackBuffer as RenderTarget Interface");

            mDevice->CreateRenderTargetView(mBackBuffers[n].Get(), nullptr, rtvHandle);
            rtvHandle.ptr += (1 * mRtvDescriptorSize);
        }
    }
}


//-------------------------------------------------------------------------------------------------
void yaget::render::Device::InitializeResources()
{
    mRootSignature = CreateRootSignature(mDevice);
}


//-------------------------------------------------------------------------------------------------
void yaget::render::Device::Resize()
{
    WaiterScoper scoper(mWaiter);

    const UINT64 fence = mFenceValue;
    HRESULT hr = mCommandQueue->Signal(mFence.Get(), fence);
    error_handlers::ThrowOnError(hr, "Could not signal DX12 Fence.");
    ++mFenceValue;

    if (mFence->GetCompletedValue() < fence)
    {
        hr = mFence->SetEventOnCompletion(fence, mFenceEvent);
        error_handlers::ThrowOnError(hr, "Could not DX12 SetEventOnCompletion");
        WaitForSingleObjectEx(mFenceEvent, INFINITE, false);
    }

    ResetBackBuffers();
    SetupSwapChain();
    InitBackBuffers();
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
