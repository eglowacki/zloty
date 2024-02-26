#include "Render/Polygons/Polygon.h"
#include "App/AppUtilities.h"
#include "Fmt/format.h"
#include "Render/Platform/D3D12MemAlloc.h"
#include "Render/Platform/DeviceDebugger.h"
#include "Render/Platform/ResourceCompiler.h"
#include "MathFacade.h"

#include <d3dx12.h>
#include <d3dcompiler.h>
#include <CommonStates.h>
#include <VertexTypes.h>

#include "Core/ErrorHandlers.h"

namespace
{
    const char* shaderSource = 
        R"( struct PSInput
            {
                float4 position : SV_POSITION;
                float4 color : COLOR;
            };

            PSInput VSMain(float4 position : SV_POSITION, float4 color : COLOR)
            {
                PSInput result;

                result.position = position;
                result.color = color;

                return result;
            }

            float4 PSMain(PSInput input) : SV_TARGET
            {
                return input.color;
            }
        )";

    const int numTriangles = 1;

    const float aspectRatio = 1.0f;
    struct Vertex
    {
        math3d::Vector3 position;
        math3d::Vector4 color;
    };

    const Vertex vertices[] = {
        { { 0.0f, 1.0f * aspectRatio, 0.0f }, { 1.0f, 0.0f, 0.0f, 0.99f } },
        { { 1.0f, -1.0f * aspectRatio, 0.0f }, { 0.0f, 1.0f, 0.0f, 0.f } },
        { { -1.0f, -1.0f * aspectRatio, 0.0f }, { 0.0f, 0.0f, 1.0f, 0.f } }
    };

    const Vertex vertices2[] = {
        { { 0.0f, 1.0f * aspectRatio, 0.0f }, { 1.0f, 0.0f, 0.0f, 0.99f } },
        { { 1.0f, 0.0f * aspectRatio, 0.0f }, { 0.0f, 1.0f, 0.0f, 0.f } },
        { { -1.0f, 0.0f * aspectRatio, 0.0f }, { 0.0f, 0.0f, 1.0f, 0.f } }
    };

    yaget::render::ComPtr<ID3D12RootSignature> CreateRootSignature(ID3D12Device* device)
    {
        CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
        rootSignatureDesc.Init(0, nullptr, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

        yaget::render::ComPtr<ID3DBlob> signature;
        yaget::render::ComPtr<ID3DBlob> error;
        HRESULT hr = ::D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error);
        yaget::error_handlers::ThrowOnError(hr, fmt::format("Could not serialize root signature. {}", error ? static_cast<const char*>(error->GetBufferPointer()) : ""));

        yaget::render::ComPtr<ID3D12RootSignature> rootSignature;
        hr = device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&rootSignature));
        yaget::error_handlers::ThrowOnError(hr, "Could not create root signature.");

        return rootSignature;
    }

}
// namespace


//-------------------------------------------------------------------------------------------------
yaget::render::Polygon::Polygon(ID3D12Device* device, D3D12MA::Allocator* allocator, bool useTwo)
    : mRootSignature{ CreateRootSignature(device) }
{
    YAGET_RENDER_SET_DEBUG_NAME(mRootSignature.Get(), fmt::format("Yaget-Poly Root Signature"));

    const size_t sourceLen = std::strlen(shaderSource);

    ResourceCompiler vertexCompiler({ shaderSource, sourceLen }, "VSMain", "vs_5_0", false /*useNewestCompiler*/);
    ResourceCompiler pixelCompiler({ shaderSource, sourceLen }, "PSMain", "ps_5_0", false /*useNewestCompiler*/);

    // Describe and create the graphics pipeline state object (PSO).
    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
    psoDesc.InputLayout = DirectX::VertexPositionColor::InputLayout;
    psoDesc.pRootSignature = mRootSignature.Get();
    psoDesc.VS = CD3DX12_SHADER_BYTECODE(vertexCompiler.GetCompiled());
    psoDesc.PS = CD3DX12_SHADER_BYTECODE(pixelCompiler.GetCompiled());
    psoDesc.RasterizerState = DirectX::CommonStates::CullCounterClockwise;
    psoDesc.BlendState = DirectX::CommonStates::Opaque;
    psoDesc.DepthStencilState = DirectX::CommonStates::DepthNone;
    psoDesc.SampleMask = UINT_MAX;
    psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    psoDesc.NumRenderTargets = 1;
    psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
    psoDesc.SampleDesc.Count = 1;

    HRESULT hr = device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&mPipelineState));
    error_handlers::ThrowOnError(hr, "Could not create Pipeline State.");
    YAGET_RENDER_SET_DEBUG_NAME(mPipelineState.Get(), fmt::format("Yaget-Poly Pipeline State"));

    const uint64_t verticesBufferSize = sizeof(vertices) * numTriangles;

    D3D12_RESOURCE_DESC resourceDesc = {};
    resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    resourceDesc.Alignment = 0;
    resourceDesc.Width = verticesBufferSize;
    resourceDesc.Height = 1;
    resourceDesc.DepthOrArraySize = 1;
    resourceDesc.MipLevels = 1;
    resourceDesc.Format = DXGI_FORMAT_UNKNOWN;
    resourceDesc.SampleDesc.Count = 1;
    resourceDesc.SampleDesc.Quality = 0;
    resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    resourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

    D3D12MA::ALLOCATION_DESC allocationDesc = {};
    allocationDesc.HeapType = D3D12_HEAP_TYPE_UPLOAD;
     
    D3D12MA::Allocation* allocation = nullptr;
    ComPtr<ID3D12Resource> triangleData;
    hr = allocator->CreateResource(
        &allocationDesc,
        &resourceDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        &allocation,
        IID_PPV_ARGS(&triangleData));
    error_handlers::ThrowOnError(hr, "Could not CreateResource from allocator.");
    YAGET_RENDER_SET_DEBUG_NAME(triangleData.Get(), fmt::format("Yaget-Poly Triangle Data"));

    mAllocation.reset(allocation);
    mAllocation->SetName(L"PolygonOneTriangle");

    const float scale = 0.5f;
    const math3d::Vector3 offset{ -(1.0f - scale), (1.0f - scale), 0.0f };

    std::vector<Vertex> scaledTriangle;
    for (auto i = 0; i < numTriangles; ++i)
    {
        auto selectedVertex0 = useTwo ? vertices2[0] : vertices[0];
        selectedVertex0.position *= scale;
        selectedVertex0.position += offset;

        auto selectedVertex1 = useTwo ? vertices2[1] : vertices[1];
        selectedVertex1.position *= scale;
        selectedVertex1.position += offset;

        auto selectedVertex2 = useTwo ? vertices2[2] : vertices[2];
        selectedVertex2.position *= scale;
        selectedVertex2.position += offset;

        scaledTriangle.push_back(selectedVertex0);
        scaledTriangle.push_back(selectedVertex1);
        scaledTriangle.push_back(selectedVertex2);
    }

    void* bufferData = nullptr;
    hr = triangleData->Map(0, nullptr, &bufferData);
    error_handlers::ThrowOnError(hr, "Could not map Polygon buffer for write.");

    memcpy(bufferData, scaledTriangle.data(), verticesBufferSize);
    triangleData->Unmap(0, nullptr);
}


//-------------------------------------------------------------------------------------------------
yaget::render::Polygon::~Polygon() = default;


//-------------------------------------------------------------------------------------------------
ID3D12GraphicsCommandList* yaget::render::Polygon::Render(ID3D12GraphicsCommandList* commandList, std::function<void(ID3D12GraphicsCommandList* commandList)> setup)
{
    YAGET_ASSERT(commandList, "commandList is null");

    ID3D12GraphicsCommandList* operatingCommandList = commandList;

    if (setup)
    {
        setup(operatingCommandList);
    }

    operatingCommandList->SetGraphicsRootSignature(mRootSignature.Get());
    operatingCommandList->SetPipelineState(mPipelineState.Get());

    auto triangleData = mAllocation->GetResource();
    const uint64_t verticesBufferSize = sizeof(vertices) * numTriangles;

    D3D12_VERTEX_BUFFER_VIEW triangleDataView;
    triangleDataView.BufferLocation = triangleData->GetGPUVirtualAddress();
    triangleDataView.SizeInBytes = verticesBufferSize;
    triangleDataView.StrideInBytes = sizeof(DirectX::VertexPositionColor);

    operatingCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    operatingCommandList->IASetVertexBuffers(0, 1, &triangleDataView);
    operatingCommandList->DrawInstanced(3 * numTriangles, 1, 0, 0);

    return operatingCommandList;
}


// sample from example of renderig triangle
#if 0

//*********************************************************
//
// Copyright (c) Microsoft. All rights reserved.
// This code is licensed under the MIT License (MIT).
// THIS CODE IS PROVIDED *AS IS* WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING ANY
// IMPLIED WARRANTIES OF FITNESS FOR A PARTICULAR
// PURPOSE, MERCHANTABILITY, OR NON-INFRINGEMENT.
//
//*********************************************************
struct PSInput
{
    float4 position : SV_POSITION;
    float4 color : COLOR;
};

PSInput VSMain(float4 position : POSITION, float4 color : COLOR)
{
    PSInput result;

    result.position = position;
    result.color = color;

    return result;
}

float4 PSMain(PSInput input) : SV_TARGET
{
    return input.color;
}


void D3D12HelloTriangle::LoadAssets()
{
    // Create an empty root signature.
    {
        CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
        rootSignatureDesc.Init(0, nullptr, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

        ComPtr<ID3DBlob> signature;
        ComPtr<ID3DBlob> error;
        ThrowIfFailed(D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error));
        ThrowIfFailed(m_device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&m_rootSignature)));
    }

    // Create the pipeline state, which includes compiling and loading shaders.
    {
        ComPtr<ID3DBlob> vertexShader;
        ComPtr<ID3DBlob> pixelShader;

#if defined(_DEBUG)
        // Enable better shader debugging with the graphics debugging tools.
        UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
        UINT compileFlags = 0;
#endif

        ThrowIfFailed(D3DCompileFromFile(GetAssetFullPath(L"shaders.hlsl").c_str(), nullptr, nullptr, "VSMain", "vs_5_0", compileFlags, 0, &vertexShader, nullptr));
        ThrowIfFailed(D3DCompileFromFile(GetAssetFullPath(L"shaders.hlsl").c_str(), nullptr, nullptr, "PSMain", "ps_5_0", compileFlags, 0, &pixelShader, nullptr));

        // Define the vertex input layout.
        D3D12_INPUT_ELEMENT_DESC inputElementDescs[] =
        {
            { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
        };

        // Describe and create the graphics pipeline state object (PSO).
        D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
        psoDesc.InputLayout = { inputElementDescs, _countof(inputElementDescs) };
        psoDesc.pRootSignature = m_rootSignature.Get();
        psoDesc.VS = CD3DX12_SHADER_BYTECODE(vertexShader.Get());
        psoDesc.PS = CD3DX12_SHADER_BYTECODE(pixelShader.Get());
        psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
        psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
        psoDesc.DepthStencilState.DepthEnable = FALSE;
        psoDesc.DepthStencilState.StencilEnable = FALSE;
        psoDesc.SampleMask = UINT_MAX;
        psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        psoDesc.NumRenderTargets = 1;
        psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
        psoDesc.SampleDesc.Count = 1;
        ThrowIfFailed(m_device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pipelineState)));
    }

    // Create the command list.
    ThrowIfFailed(m_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_commandAllocator.Get(), m_pipelineState.Get(), IID_PPV_ARGS(&m_commandList)));

    // Command lists are created in the recording state, but there is nothing
    // to record yet. The main loop expects it to be closed, so close it now.
    ThrowIfFailed(m_commandList->Close());

    // Create the vertex buffer.
    {
        // Define the geometry for a triangle.
        Vertex triangleVertices[] =
        {
            { { 0.0f, 0.25f * m_aspectRatio, 0.0f }, { 1.0f, 0.0f, 0.0f, 1.0f } },
            { { 0.25f, -0.25f * m_aspectRatio, 0.0f }, { 0.0f, 1.0f, 0.0f, 1.0f } },
            { { -0.25f, -0.25f * m_aspectRatio, 0.0f }, { 0.0f, 0.0f, 1.0f, 1.0f } }
        };

        const UINT vertexBufferSize = sizeof(triangleVertices);

        // Note: using upload heaps to transfer static data like vert buffers is not 
        // recommended. Every time the GPU needs it, the upload heap will be marshalled 
        // over. Please read up on Default Heap usage. An upload heap is used here for                                                                                                   0    7
        // code simplicity and because there are very few verts to actually transfer.
        ThrowIfFailed(m_device->CreateCommittedResource(
            &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
            D3D12_HEAP_FLAG_NONE,
            &CD3DX12_RESOURCE_DESC::Buffer(vertexBufferSize),
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_PPV_ARGS(&m_vertexBuffer)));

        // Copy the triangle data to the vertex buffer.
        UINT8* pVertexDataBegin;
        CD3DX12_RANGE readRange(0, 0);        // We do not intend to read from this resource on the CPU.
        ThrowIfFailed(m_vertexBuffer->Map(0, &readRange, reinterpret_cast<void**>(&pVertexDataBegin)));
        memcpy(pVertexDataBegin, triangleVertices, sizeof(triangleVertices));
        m_vertexBuffer->Unmap(0, nullptr);

        // Initialize the vertex buffer view.
        m_vertexBufferView.BufferLocation = m_vertexBuffer->GetGPUVirtualAddress();
        m_vertexBufferView.StrideInBytes = sizeof(Vertex);
        m_vertexBufferView.SizeInBytes = vertexBufferSize;
    }

    // Create synchronization objects and wait until assets have been uploaded to the GPU.
    {
        ThrowIfFailed(m_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence)));
        m_fenceValue = 1;

        // Create an event handle to use for frame synchronization.
        m_fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
        if (m_fenceEvent == nullptr)
        {
            ThrowIfFailed(HRESULT_FROM_WIN32(GetLastError()));
        }

        // Wait for the command list to execute; we are reusing the same command 
        // list in our main loop but for now, we just want to wait for setup to 
        // complete before continuing.
        WaitForPreviousFrame();
    }
}

// needs to do this in render
    m_commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    m_commandList->IASetVertexBuffers(0, 1, &m_vertexBufferView);
    m_commandList->DrawInstanced(3, 1, 0, 0);

#endif // 0