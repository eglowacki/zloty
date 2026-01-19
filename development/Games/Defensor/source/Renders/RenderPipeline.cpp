#include "Core/ErrorHandlers.h"
#include "Render/Platform/DeviceDebugger.h"
#include "RenderPipeline.h"
#include <CommonStates.h>
#include <d3dx12.h>
#include <Fmt/format.h>
#include <VertexTypes.h>
#include "Streams/Buffers.h"


namespace
{
 
    template<typename T>
    yaget::render::ComPtr<ID3D12PipelineState> CreatePipeline(ID3D12Device* device, ID3D12RootSignature* rootSignature, yaget::io::Buffer vertexShaderBuffer, yaget::io::Buffer pixelShaderBuffer, yaget::io::Buffer dataBlob)
    {
        using namespace yaget;

        // Describe and create the graphics pipeline state object (PSO).
        D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
        if (io::size_data(dataBlob))
        {
            psoDesc.CachedPSO.pCachedBlob = io::cast_data<const char>(dataBlob);
            psoDesc.CachedPSO.CachedBlobSizeInBytes = io::size_data(dataBlob);
        }

        psoDesc.InputLayout = T::InputLayout;
        psoDesc.pRootSignature = rootSignature;
        psoDesc.VS = CD3DX12_SHADER_BYTECODE(io::cast_data<const char*>(vertexShaderBuffer), io::size_data(vertexShaderBuffer));
        psoDesc.PS = CD3DX12_SHADER_BYTECODE(io::cast_data<const char*>(pixelShaderBuffer), io::size_data(pixelShaderBuffer));
        psoDesc.RasterizerState = DirectX::CommonStates::CullCounterClockwise;
        psoDesc.BlendState = DirectX::CommonStates::Opaque;
        psoDesc.DepthStencilState = DirectX::CommonStates::DepthNone;
        psoDesc.SampleMask = UINT_MAX;
        psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        psoDesc.NumRenderTargets = 1;
        psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
        psoDesc.SampleDesc.Count = 1;

        yaget::render::ComPtr<ID3D12PipelineState> pipelineState;
        HRESULT hr = device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pipelineState));
        error_handlers::ThrowOnError(hr, "Could not create Pipeline State.");
        YAGET_RENDER_SET_DEBUG_NAME(pipelineState.Get(), fmt::format("Pipeline State"));

        return pipelineState;
    }

}


//-------------------------------------------------------------------------------------------------
defensor::render::RenderPipeline::RenderPipeline(ID3D12Device* device, io::VirtualTransportSystem& vts)
    : mDevice(device)
    , mVTS(vts)
    , mCache(mVTS, yaget::io::VirtualTransportSystem::Section("Caches@Pipelines"))
{
}


//-------------------------------------------------------------------------------------------------
defensor::render::RenderPipeline::~RenderPipeline() = default;


//-------------------------------------------------------------------------------------------------
ID3D12PipelineState* defensor::render::RenderPipeline::GetPipeline(uint64_t pipeType, ID3D12RootSignature* rootSignature, yaget::io::Buffer vertexShaderBuffer, yaget::io::Buffer pixelShaderBuffer)
{
    YAGET_ASSERT(pipeType < yaget::render::AssetCache::TypeToTag.size(), "pipeType '%d' is not valid for pipeline", pipeType);

    if (auto it = mPipelines.find(pipeType); it != mPipelines.end())
    {
        return it->second.Get();
    }

    io::Buffer cachedData = mCache.GetCachedAsset(yaget::render::AssetCache::TypeToTag[pipeType]);

    auto pipe = CreatePipeline<DirectX::VertexPositionColor>(mDevice, rootSignature, vertexShaderBuffer, pixelShaderBuffer, cachedData);
    mPipelines.insert({pipeType, pipe});

    if (!io::size_data(cachedData))
    {
        yaget::render::ComPtr<ID3DBlob> pipeBlob;
        HRESULT hr = pipe->GetCachedBlob(&pipeBlob);
        error_handlers::ThrowOnError(hr, fmt::format("Could not get ID3D12PipelineState '{}' as a blob to save it", pipeType));

        mCache.SaveCachedAsset(yaget::render::AssetCache::TypeToTag[pipeType], io::CreateBuffer(static_cast<const char*>(pipeBlob->GetBufferPointer()), pipeBlob->GetBufferSize()));
    }

    return pipe.Get();
}
