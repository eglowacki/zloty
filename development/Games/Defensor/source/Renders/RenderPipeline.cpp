#include "RenderPipeline.h"

#include <CommonStates.h>
#include "Render/Platform/ResourceCompiler.h"
#include <d3dx12.h>
#include <VertexTypes.h>
#include <Fmt/format.h>

#include "Core/ErrorHandlers.h"
#include "Render/Platform/DeviceDebugger.h"


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
  
    template<typename T>
    yaget::render::ComPtr<ID3D12PipelineState> CreatePipeline(ID3D12Device* device, ID3D12RootSignature* rootSignature)
    {
        yaget::render::ComPtr<ID3D12PipelineState> pipelineState;

        const size_t sourceLen = std::strlen(shaderSource);

        yaget::render::ResourceCompiler vertexCompiler({ shaderSource, sourceLen }, "VSMain", "vs_5_0", false /*useNewestCompiler*/);
        yaget::render::ResourceCompiler pixelCompiler({ shaderSource, sourceLen }, "PSMain", "ps_5_0", false /*useNewestCompiler*/);

        // Describe and create the graphics pipeline state object (PSO).
        D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
        psoDesc.InputLayout = T::InputLayout;
        psoDesc.pRootSignature = rootSignature;
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

        HRESULT hr = device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pipelineState));
        yaget::error_handlers::ThrowOnError(hr, "Could not create Pipeline State.");
        YAGET_RENDER_SET_DEBUG_NAME(pipelineState.Get(), fmt::format("Pipeline State"));

        return pipelineState;
    }
}


defensor::render::RenderPipeline::RenderPipeline(ID3D12Device* device)
    : mDevice(device)
{
}


defensor::render::RenderPipeline::~RenderPipeline() = default;


ID3D12PipelineState* defensor::render::RenderPipeline::GetPipeline(uint64_t pipeType, ID3D12RootSignature* rootSignature)
{
    if (auto it = mPipelines.find(pipeType); it != mPipelines.end())
    {
        return it->second.Get();
    }

    auto pipe = CreatePipeline<DirectX::VertexPositionColor>(mDevice, rootSignature);
    mPipelines.insert({pipeType, pipe});
    return pipe.Get();
}
