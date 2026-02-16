#include "Render/Platform/ResourceCompiler.h"
#include "App/AppUtilities.h"
#include "Fmt/format.h"

#include <d3dcompiler.h>
#include <d3dx12.h>

#include <d3d12shader.h>    // Shader reflection.
#include <dxcapi.h>         // Be sure to link with dxcompiler.lib.
#include <ranges>

#include "StringHelpers.h"
#include "Core/ErrorHandlers.h"
#include "Exception/Exception.h"
#include "Platform/Support.h"

// New compiler for shaders
// https://github.com/microsoft/DirectXShaderCompiler/wiki/Using-dxc.exe-and-dxcompiler.dll

namespace
{

    const yaget::render::ResourceReflector::IndexMap MakeIndexMap(const yaget::render::ResourceReflector::RootParameters& rootParameters)
    {
        yaget::render::ResourceReflector::IndexMap result;
        uint32_t slot = 0;
        for (auto value : rootParameters)
        {
            result[value.mName] = slot++;
        }

        return result;
    }
}


//-------------------------------------------------------------------------------------------------
const yaget::render::ResourceReflector::RootDescResult yaget::render::ResourceReflector::MakeRootSignature(const RootParameters& rootParameters)
{
    RootDescResult rootResult;

    rootResult.mRootParameters = rootParameters | std::views::transform([](const auto& element)
        {
            return element.mParameter;
        }) | std::ranges::to<std::vector<D3D12_ROOT_PARAMETER1>>();

    rootResult.mRootSignatureDesc =
    {
        .Version = D3D_ROOT_SIGNATURE_VERSION_1_1,
        .Desc_1_1
        {
            .NumParameters = static_cast<uint32_t>(rootResult.mRootParameters.size()),
            .pParameters = rootResult.mRootParameters.data(),
            .NumStaticSamplers = 0u,
            .pStaticSamplers = nullptr,
            .Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT,
        },
    };

    rootResult.mIndexMap = MakeIndexMap(rootParameters);

    return rootResult;
}


//-------------------------------------------------------------------------------------------------
void yaget::render::ResourceReflector::MakeRootSignature(ResourceReflector* additionalReflector, DescriptionCallback descriptionCallback)
{
    RootParameters rootParameters;
    GenerateSignature(rootParameters);
    if (additionalReflector)
    {
        additionalReflector->GenerateSignature(rootParameters);
    }

    const RootDescResult rootResult = ResourceReflector::MakeRootSignature(rootParameters);

    descriptionCallback(rootResult);
}


//-------------------------------------------------------------------------------------------------
yaget::render::ResourceCompiler::ResourceCompiler()
{
    HRESULT hr = DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&mUtils));
    error_handlers::ThrowOnError(hr, "Could not get DX12 DxcUtil object");

    hr = DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&mCompiler));
    error_handlers::ThrowOnError(hr, "Could not create DxcCompiler3 object");
}


//-------------------------------------------------------------------------------------------------
yaget::render::ResourceCompiler::~ResourceCompiler() = default;


//-------------------------------------------------------------------------------------------------
namespace
{
    struct ShaderParameters
    {
        ShaderParameters(const yaget::Strings& parameters)
        {
            mShaderArguments = parameters | std::views::transform([](const std::string& element)
                {
                    return yaget::conv::utf8_to_wide(element);
                }) | std::ranges::to<std::vector<std::wstring>>();

            mArguments = mShaderArguments | std::views::transform([](const std::wstring& element)
                {
                    return element.c_str();
                }) | std::ranges::to<std::vector<LPCWSTR>>();
        }

        std::vector<LPCWSTR> mArguments;

    private:
        std::vector<std::wstring> mShaderArguments;
        
    };
}


//-------------------------------------------------------------------------------------------------
yaget::render::ResourceCompiler::CompileResult yaget::render::ResourceCompiler::Compile(io::BufferView data, const Strings& parameters) const
{
    CompileResult compiledResult;
    
    ShaderParameters shaderParameters(parameters);

    constexpr uint32_t codePage = CP_UTF8;
    DxcText dxBuffer{ io::cast_data<const char>(data), io::size_data(data), codePage };
    ComPtr<IDxcResult> result;
    HRESULT hr = mCompiler->Compile(&dxBuffer, shaderParameters.mArguments.data(), static_cast<UINT32>(shaderParameters.mArguments.size()), nullptr, IID_PPV_ARGS(&result));
    error_handlers::ThrowOnError(hr, fmt::format("Could not execute compiler for shader with params: '{}'", conv::Combine(parameters, ", ")));

    ComPtr<IDxcBlobUtf8> errors;
    hr = result->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&errors), nullptr);
    error_handlers::ThrowOnError(hr, "Could not get output for shader from compiled results.");

    if (errors && errors->GetStringLength())
    {
        error_handlers::ThrowOnError(hr, fmt::format("Did not compile shader. {}", errors->GetStringPointer()));
    }

    ComPtr<IDxcBlob> shaderBin;
    hr = result->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&shaderBin), nullptr);
    error_handlers::ThrowOnError(hr, "Could not get bin shader from compiled results.");

    const auto bufferSize = shaderBin->GetBufferSize();
    compiledResult.first = io::CreateBuffer(static_cast<const char*>(shaderBin->GetBufferPointer()), bufferSize);

    try
    {
        compiledResult.second = std::make_shared<ResourceReflector>(mUtils.Get(), io::cast_to_view(compiledResult.first));
    }
    catch (const yaget::ex::bad_init& ex)
    {
        YLOG_ERROR("COMP", "Did not get reflection from shader. Error: %s", ex.what());
    }

    return compiledResult;
}


//-------------------------------------------------------------------------------------------------
yaget::render::ResourceReflector::Ptr yaget::render::ResourceCompiler::Decompile(io::BufferView data) const
{
    auto reflector = std::make_shared<ResourceReflector>(mUtils.Get(), data);
    return reflector;
}


//-------------------------------------------------------------------------------------------------
yaget::render::ResourceReflector::ResourceReflector(IDxcUtils* utils, io::BufferView buffer)
{
    void* bufferPtr = io::cast_data<void>(buffer);
    auto bufferSize = io::size_data(buffer);

    const DxcBuffer reflectionBuffer{ .Ptr = bufferPtr, .Size = bufferSize, .Encoding = 0, };
    HRESULT hr = utils->CreateReflection(&reflectionBuffer, IID_PPV_ARGS(&mShaderReflection));
    error_handlers::ThrowOnError(hr, "Could not get Shader Reflection from compiled shader.");
}


//-------------------------------------------------------------------------------------------------
D3D12_SHADER_VISIBILITY yaget::render::ResourceReflector::GeneratePins(uint32_t shaderType, const D3D12_SHADER_DESC& shaderDesc)
{
    using namespace yaget;
    D3D12_SHADER_VISIBILITY returnValue = D3D12_SHADER_VISIBILITY_ALL;

    //---------------------------------------
    // process shaders first (for now we only deal with vertex or pixel ones.
    switch (shaderType)
    {
    case D3D12_SHVER_VERTEX_SHADER:
    case D3D12_SHVER_PIXEL_SHADER:
        {
            auto& shaderInputs = mShaderInputs = {};
            auto& shaderOutputs = mShaderOutputs = {};

            auto paramDesc = [](uint32_t parameterIndex, auto descGetter)
            {
                D3D12_SIGNATURE_PARAMETER_DESC signatureParameterDesc{};
                HRESULT hr = descGetter(parameterIndex, &signatureParameterDesc);
                error_handlers::ThrowOnError(hr, fmt::format("Could not get input Parameter Description from compiled vertex shader. Parameter Index: {}", parameterIndex));

                return signatureParameterDesc;
            };

            for (const uint32_t parameterIndex : std::views::iota(0u, shaderDesc.InputParameters))
            {
                auto signatureParameterDesc = paramDesc(parameterIndex, [&shaderReflection = mShaderReflection](UINT parameterIndex, D3D12_SIGNATURE_PARAMETER_DESC *desc)
                {
                    return shaderReflection->GetInputParameterDesc(parameterIndex, desc);
                });

                shaderInputs.push_back({ .mName = signatureParameterDesc.SemanticName, .mIndex = signatureParameterDesc.SemanticIndex });
            }

            for (const uint32_t parameterIndex : std::views::iota(0u, shaderDesc.OutputParameters))
            {
                auto signatureParameterDesc = paramDesc(parameterIndex, [&shaderReflection = mShaderReflection](UINT parameterIndex, D3D12_SIGNATURE_PARAMETER_DESC *desc)
                {
                    return shaderReflection->GetOutputParameterDesc(parameterIndex, desc);
                });

                shaderOutputs.push_back({ .mName = signatureParameterDesc.SemanticName, .mIndex = signatureParameterDesc.SemanticIndex });
            }

            returnValue = shaderType == D3D12_SHVER_VERTEX_SHADER ? D3D12_SHADER_VISIBILITY_VERTEX : D3D12_SHADER_VISIBILITY_PIXEL;
        }
        break;
    default:
        YLOG_ERROR("COMP", "Unsupported shader type: '%d'", shaderType);
    }

    return returnValue;
}


//-------------------------------------------------------------------------------------------------
void yaget::render::ResourceReflector::GenerateSignature(RootParameters& rootParameters)
{
    D3D12_SHADER_DESC shaderDesc{};
    HRESULT hr = mShaderReflection->GetDesc(&shaderDesc);
    error_handlers::ThrowOnError(hr, "Could not get Shader Description from compiled shader.");

    mShaderType = (shaderDesc.Version & 0xFFFF0000) >> 16;
    mMajorVersion = (shaderDesc.Version & 0x000000F0) >> 4;
    mMinorVersion = (shaderDesc.Version & 0x0000000F);

    mShaderVisibility = GeneratePins(mShaderType, shaderDesc);

    //------------------------------------------------------------------
    // next let's go over bounded resources
    for (const uint32_t i : std::views::iota(0u, shaderDesc.BoundResources))
    {
        D3D12_SHADER_INPUT_BIND_DESC shaderInputBindDesc{};
        hr = mShaderReflection->GetResourceBindingDesc(i, &shaderInputBindDesc);
        error_handlers::ThrowOnError(hr, fmt::format("Could not get Resource Binding Description from compiled vertex shader. BoundResources Index: {}", i));

        switch (shaderInputBindDesc.Type)
        {
        case D3D_SIT_CBUFFER:
            {
                ID3D12ShaderReflectionConstantBuffer* shaderReflectionConstantBuffer = mShaderReflection->GetConstantBufferByIndex(i);

                D3D12_SHADER_BUFFER_DESC constantBufferDesc{};
                hr = shaderReflectionConstantBuffer->GetDesc(&constantBufferDesc);
                error_handlers::ThrowOnError(hr, fmt::format("Could not get Constant Buffer Description from compiled vertex shader. BoundResources Index: {}", i));

                // NOTE(eg) we may want to consider having path for small (one matrix?) root const buffer
                D3D12_ROOT_PARAMETER1 rootParameter = {};

                if (constantBufferDesc.Variables * constantBufferDesc.Size == sizeof(float) * 16)
                {
                    rootParameter = D3D12_ROOT_PARAMETER1
                    {
                        .ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS,
                        .Constants
                        {
                            .ShaderRegister = shaderInputBindDesc.BindPoint,
                            .RegisterSpace = shaderInputBindDesc.Space,
                            .Num32BitValues = 16,
                        },
                        .ShaderVisibility = mShaderVisibility,
                    };
                }
                else
                {
                    rootParameter = D3D12_ROOT_PARAMETER1
                    {
                        .ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV,
                        .Descriptor
                        {
                            .ShaderRegister = shaderInputBindDesc.BindPoint,
                            .RegisterSpace = shaderInputBindDesc.Space,
                            .Flags = D3D12_ROOT_DESCRIPTOR_FLAG_NONE,
                        },
                        .ShaderVisibility = mShaderVisibility,
                    };
                }

                rootParameters.push_back({ .mParameter = rootParameter, .mName = shaderInputBindDesc.Name });
            }
            break;
        case D3D_SIT_TEXTURE:
            {
                rootParameters.push_back({});
                auto& parameter = rootParameters.back();
                const CD3DX12_DESCRIPTOR_RANGE1 srvRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV,
                    1u,
                    shaderInputBindDesc.BindPoint,
                    shaderInputBindDesc.Space,
                    D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC);

                parameter.mDescriptorRangesScratchPad.push_back(srvRange);
                //std::vector<CD3DX12_DESCRIPTOR_RANGE1> descriptorRanges;
                //descriptorRanges.push_back(srvRange);

                const D3D12_ROOT_PARAMETER1 rootParameter
                {
                    .ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE,
                    .DescriptorTable
                    {
                        .NumDescriptorRanges = 1u,
                        .pDescriptorRanges = &parameter.mDescriptorRangesScratchPad.back(),
                    },
                    .ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL,
                };

                rootParameters.push_back({ .mParameter = rootParameter, .mName = shaderInputBindDesc.Name });
            }
            break;
        default:
            YLOG_ERROR("COMP", "Unsupported Bound Resources type: '%d'", shaderInputBindDesc.Type);
        }
    }
}
