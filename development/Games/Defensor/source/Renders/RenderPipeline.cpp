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
    yaget::render::ComPtr<ID3D12PipelineState> CreatePipeline(ID3D12Device* device, ID3D12RootSignature* rootSignature, yaget::io::Buffer vertexShaderBuffer, yaget::io::Buffer pixelShaderBuffer)
    {
        using namespace yaget;

        yaget::render::ComPtr<ID3D12PipelineState> pipelineState;

        // Describe and create the graphics pipeline state object (PSO).
        D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
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

        HRESULT hr = device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pipelineState));
        error_handlers::ThrowOnError(hr, "Could not create Pipeline State.");
        YAGET_RENDER_SET_DEBUG_NAME(pipelineState.Get(), fmt::format("Pipeline State"));

        return pipelineState;
    }
}


//-------------------------------------------------------------------------------------------------
defensor::render::RenderPipeline::RenderPipeline(ID3D12Device* device)
    : mDevice(device)
{
}


//-------------------------------------------------------------------------------------------------
defensor::render::RenderPipeline::~RenderPipeline() = default;


//-------------------------------------------------------------------------------------------------
ID3D12PipelineState* defensor::render::RenderPipeline::GetPipeline(uint64_t pipeType, ID3D12RootSignature* rootSignature, yaget::io::Buffer vertexShaderBuffer, yaget::io::Buffer pixelShaderBuffer)
{
    if (auto it = mPipelines.find(pipeType); it != mPipelines.end())
    {
        return it->second.Get();
    }

    auto pipe = CreatePipeline<DirectX::VertexPositionColor>(mDevice, rootSignature, vertexShaderBuffer, pixelShaderBuffer);
    mPipelines.insert({pipeType, pipe});
    return pipe.Get();
}
