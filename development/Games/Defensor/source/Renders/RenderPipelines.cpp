#include "Core/ErrorHandlers.h"
#include "Render/Platform/DeviceDebugger.h"
#include "RenderPipelines.h"
#include <CommonStates.h>
#include <d3dx12.h>
#include <Fmt/format.h>
#include <VertexTypes.h>

#include "Parsers/DependencyGraph.h"
#include "Streams/Buffers.h"


namespace
{
    D3D12_DEPTH_STENCIL_DESC DepthStencilOn
    {
        .DepthEnable = TRUE,
        .DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL,
        .DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL,
        .StencilEnable = TRUE,
        .StencilReadMask = D3D12_DEFAULT_STENCIL_READ_MASK,
        .StencilWriteMask = D3D12_DEFAULT_STENCIL_WRITE_MASK,
        .FrontFace =
        {
            .StencilFailOp = D3D12_STENCIL_OP_KEEP,
            .StencilDepthFailOp = D3D12_STENCIL_OP_KEEP,
            .StencilPassOp = D3D12_STENCIL_OP_KEEP,
            .StencilFunc = D3D12_COMPARISON_FUNC_ALWAYS
        },
        .BackFace =
        {
            .StencilFailOp = D3D12_STENCIL_OP_KEEP,
            .StencilDepthFailOp = D3D12_STENCIL_OP_KEEP,
            .StencilPassOp = D3D12_STENCIL_OP_KEEP,
            .StencilFunc = D3D12_COMPARISON_FUNC_ALWAYS
        }
    };


    D3D12_BLEND_DESC GetBlendState(yaget::render::AssetCacheType assetType)
    {
        D3D12_BLEND_DESC blendState = DirectX::CommonStates::Opaque;
        if (static_cast<bool>(assetType & yaget::render::AssetCacheType::BlendModeAlpha))
        {
            blendState = DirectX::CommonStates::AlphaBlend;
        }
        else if (static_cast<bool>(assetType & yaget::render::AssetCacheType::BlendModeAdditive))
        {
            blendState = DirectX::CommonStates::Additive;
        }
        else if (static_cast<bool>(assetType & yaget::render::AssetCacheType::BlendModeNonPremultiplied))
        {
            blendState = DirectX::CommonStates::NonPremultiplied;
        }
        return blendState;
    }

    D3D12_RASTERIZER_DESC GetRasterizeState(yaget::render::AssetCacheType assetType)
    {
        D3D12_RASTERIZER_DESC rasterizerState = DirectX::CommonStates::CullNone;
        if (static_cast<bool>(assetType & yaget::render::AssetCacheType::RasterizerStateClockwise))
        {
            rasterizerState = DirectX::CommonStates::CullClockwise;
        }
        if (static_cast<bool>(assetType & yaget::render::AssetCacheType::RasterizerStateCounterClockwise))
        {
            rasterizerState = DirectX::CommonStates::CullCounterClockwise;
        }
        else if (static_cast<bool>(assetType & yaget::render::AssetCacheType::RasterizerStateWireframe))
        {
            rasterizerState = DirectX::CommonStates::Wireframe;
        }
        return rasterizerState;
    }

    D3D12_DEPTH_STENCIL_DESC GetDepthState(yaget::render::AssetCacheType assetType)
    {
        D3D12_DEPTH_STENCIL_DESC depthStencilState = DirectX::CommonStates::DepthNone;
        if (static_cast<bool>(assetType & yaget::render::AssetCacheType::DepthStateOn))
        {
            depthStencilState = DirectX::CommonStates::DepthDefault;
        }
        else if (static_cast<bool>(assetType & yaget::render::AssetCacheType::DepthStateRead))
        {
            depthStencilState = DirectX::CommonStates::DepthRead;
        }
        else if (static_cast<bool>(assetType & yaget::render::AssetCacheType::DepthStencilStateOn))
        {
            depthStencilState = DepthStencilOn;
        }
        return depthStencilState;
    }

    D3D12_PRIMITIVE_TOPOLOGY_TYPE GetPrimitiveTopologyType(yaget::render::AssetCacheType assetType)
    {
        D3D12_PRIMITIVE_TOPOLOGY_TYPE topologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        if (static_cast<bool>(assetType & yaget::render::AssetCacheType::TopologyStatePoint))
        {
            topologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;
        }
        else if (static_cast<bool>(assetType & yaget::render::AssetCacheType::TopologyStateLine))
        {
            topologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
        }
        return topologyType;
    }

    DXGI_FORMAT GetRenderTargetFormat(yaget::render::AssetCacheType assetType)
    {
        DXGI_FORMAT format = DXGI_FORMAT_R8G8B8A8_UNORM;
        if (static_cast<bool>(assetType & yaget::render::AssetCacheType::RTVFormatRGBA16F))
        {
            format = DXGI_FORMAT_R16G16B16A16_FLOAT;
        }
        else if (static_cast<bool>(assetType & yaget::render::AssetCacheType::RTVFormatRGBA32F))
        {
            format = DXGI_FORMAT_R32G32B32A32_FLOAT;
        }
        else if (static_cast<bool>(assetType & yaget::render::AssetCacheType::DSVFormatD24S8))
        {
            format = DXGI_FORMAT_D24_UNORM_S8_UINT;
        }
        return format;
    }

    uint32_t GetNumRenderTargets(yaget::render::AssetCacheType assetType)
    {
        uint32_t numRenderTargets = 1;
        if (static_cast<bool>(assetType & yaget::render::AssetCacheType::NumRTVTargetsTwo))
        {
            numRenderTargets = 2;
        }
        else if (static_cast<bool>(assetType & yaget::render::AssetCacheType::NumRTVTargetsThree))
        {
            numRenderTargets = 3;
        }
        else if (static_cast<bool>(assetType & yaget::render::AssetCacheType::NumRTVTargetsFour))
        {
            numRenderTargets = 4;
        }
        return numRenderTargets;
    }

    template<typename T>
    yaget::render::ComPtr<ID3D12PipelineState> CreatePipeline(const yaget::io::Tag& tag, ID3D12Device* device, ID3D12RootSignature* rootSignature, yaget::io::Buffer vertexShaderBuffer, yaget::io::Buffer pixelShaderBuffer, yaget::io::Buffer& dataBlob)
    {
        using namespace yaget;

        auto assetType = render::AssetCache::operator[](tag);

        D3D12_BLEND_DESC blendState = GetBlendState(assetType);
        D3D12_RASTERIZER_DESC rasterizerState = GetRasterizeState(assetType);
        D3D12_DEPTH_STENCIL_DESC depthState = GetDepthState(assetType);
        D3D12_PRIMITIVE_TOPOLOGY_TYPE primitiveTopology = GetPrimitiveTopologyType(assetType);
        DXGI_FORMAT colorFormat = GetRenderTargetFormat(assetType);
        uint32_t numTargets = GetNumRenderTargets(assetType);

        // Describe and create the graphics pipeline state object (PSO).
        D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
        if (io::size_data(dataBlob))
        {
            psoDesc.CachedPSO.pCachedBlob = io::cast_data<const char>(dataBlob);
            psoDesc.CachedPSO.CachedBlobSizeInBytes = io::size_data(dataBlob);
        }

        psoDesc.InputLayout = T::InputLayout;
        psoDesc.pRootSignature = rootSignature;
        psoDesc.VS = CD3DX12_SHADER_BYTECODE(io::cast_data<const char>(vertexShaderBuffer), io::size_data(vertexShaderBuffer));
        psoDesc.PS = CD3DX12_SHADER_BYTECODE(io::cast_data<const char>(pixelShaderBuffer), io::size_data(pixelShaderBuffer));
        psoDesc.RasterizerState = rasterizerState;
        psoDesc.BlendState = blendState;
        psoDesc.DepthStencilState = depthState;
        psoDesc.SampleMask = UINT_MAX;
        psoDesc.PrimitiveTopologyType = primitiveTopology;
        psoDesc.NumRenderTargets = numTargets;
        for (uint32_t i = 0; i < numTargets; ++i)
        {
            psoDesc.RTVFormats[i] = colorFormat;
        }
        psoDesc.SampleDesc.Count = 1;

        yaget::render::ComPtr<ID3D12PipelineState> pipelineState;
        HRESULT hr = device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pipelineState));
        error_handlers::ThrowOnError(hr, "Could not create Pipeline State.");
        YAGET_RENDER_SET_DEBUG_NAME(pipelineState.Get(), fmt::format("Pipeline State"));

        if (!io::size_data(dataBlob))
        {
            yaget::render::ComPtr<ID3DBlob> pipeBlob;
            hr = pipelineState->GetCachedBlob(&pipeBlob);
            error_handlers::ThrowOnError(hr, fmt::format("Could not get ID3D12PipelineState as a blob to save it"));

            dataBlob = io::CreateBuffer(static_cast<const char*>(pipeBlob->GetBufferPointer()), pipeBlob->GetBufferSize());
        }

        YLOG_INFO("COMP", "Created pipeline state for: '%s'", yaget::conv::Convertor<yaget::io::Tag>::ToString(tag).c_str());
        return pipelineState;
    }

}


//-------------------------------------------------------------------------------------------------
defensor::render::RenderPipelines::RenderPipelines(ID3D12Device* device, io::VirtualTransportSystem& vts)
    : CacheWatcher(vts, yaget::io::VirtualTransportSystem::Section("Caches@Pipelines"))
    , mDevice(device)
{
}


//-------------------------------------------------------------------------------------------------
defensor::render::RenderPipelines::~RenderPipelines() = default;


//-------------------------------------------------------------------------------------------------
ID3D12PipelineState* defensor::render::RenderPipelines::GetPipeline(const yaget::io::Tag& tag, ID3D12RootSignature* rootSignature, yaget::io::Buffer vertexShaderBuffer, yaget::io::Buffer pixelShaderBuffer)
{
    YAGET_ASSERT(tag.IsValid(), "Tag: '%s:%s' is not valid.", yaget::conv::Convertor<yaget::Guid>::ToString(tag.mGuid).c_str(), yaget::conv::Convertor<yaget::io::Tag>::ToString(tag).c_str());

    std::lock_guard mutexLocker(mMutex);

    auto result = GetAsset(tag, [this, rootSignature, vertexShaderBuffer, pixelShaderBuffer](auto tag, auto& cachedData)
    {
        return CreatePipeline<DirectX::VertexPositionColor>(tag, mDevice, rootSignature, vertexShaderBuffer, pixelShaderBuffer, cachedData);
    });

    return result.Get();
}


//-------------------------------------------------------------------------------------------------
ID3D12PipelineState* defensor::render::RenderPipelines::GetPipeline(const io::Tag& tag)
{
    return GetPipeline(tag, nullptr, {}, {});
}
