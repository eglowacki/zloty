#include "Render/Platform/ResourceCompiler.h"
#include "App/AppUtilities.h"
#include "Core/ErrorHandlers.h"
#include "Exception/Exception.h"
#include "magic_enum/magic_enum.hpp"
#include "Platform/Support.h"
#include "StringHelpers.h"

#include <d3d12shader.h>    // Shader reflection.
#include <d3dcompiler.h>
#include <d3dx12.h>
#include <dxcapi.h>         // Be sure to link with dxcompiler.lib.
#include <ranges>


// New compiler for shaders
// https://github.com/microsoft/DirectXShaderCompiler/wiki/Using-dxc.exe-and-dxcompiler.dll

namespace
{
    //-------------------------------------------------------------------------------------------------
    struct ReflectorVariable
    {
        yaget::render::constant_shader_types::ConstantTypes mType;
        yaget::render::constant_shader_types::ConstantLayout mLayout;
    };


    //-------------------------------------------------------------------------------------------------
    bool operator==(const ReflectorVariable& lhs, const ReflectorVariable& rhs)
    {
        return lhs.mType == rhs.mType && lhs.mLayout == rhs.mLayout;
    }


    //-------------------------------------------------------------------------------------------------
    bool operator!=(const ReflectorVariable& lhs, const ReflectorVariable& rhs)
    {
        return !(lhs == rhs);
    }


    //-------------------------------------------------------------------------------------------------
    void to_json(nlohmann::json& j, const ReflectorVariable& reflectorVariable)
    {
        j["Type"] = magic_enum::enum_name(reflectorVariable.mType);
        j["Layout"] = magic_enum::enum_name(reflectorVariable.mLayout);
    }


    //-------------------------------------------------------------------------------------------------
    void from_json(const nlohmann::json& j, ReflectorVariable& reflectorVariable)
    {
        using namespace yaget::render;

        auto typeName = yaget::json::GetValue(j, "Type", std::string{});
        auto enumValueType = magic_enum::enum_cast<constant_shader_types::ConstantTypes>(typeName);
        if (enumValueType.has_value())
        {
            reflectorVariable.mType = enumValueType.value();
        }

        auto layoutName = yaget::json::GetValue(j, "Layout", std::string{});
        auto enumValueLayout = magic_enum::enum_cast<constant_shader_types::ConstantLayout>(layoutName);
        if (enumValueLayout.has_value())
        {
            reflectorVariable.mLayout = enumValueLayout.value();
        }
    }


    //-------------------------------------------------------------------------------------------------
    using ReflectorVariables = std::map<std::string, ReflectorVariable>;
    ReflectorVariables ReflectorVariableMappings = {
        //{ "WorldViewProjection", { yaget::render::constant_shader_types::ConstantTypes::WorldViewProjection, yaget::render::constant_shader_types::ConstantLayout::Matrix4x4 }},
        //{ "Time", { yaget::render::constant_shader_types::ConstantTypes::Time, yaget::render::constant_shader_types::ConstantLayout::Float }}
    };

    //-------------------------------------------------------------------------------------------------
    const yaget::render::ResourceReflector::IndexMap MakeIndexMap(const yaget::render::ResourceReflector::RootParameters& rootParameters)
    {
        using namespace yaget::render;

        ResourceReflector::IndexMap result;
        uint32_t slot = 0;
        for (const auto& value : rootParameters)
        {
            if (auto it = ReflectorVariableMappings.find(value.mVariableTypeName); it != ReflectorVariableMappings.end())
            {
                constant_shader_types::VariableType variableType{};

                variableType.mType = it->second.mType;
                variableType.mLayout = it->second.mLayout;
                variableType.mTypeName = value.mVariableTypeName;
                variableType.mVariableName = value.mVariableName;
                variableType.mOffset = slot++;

                switch (value.mParameter.ParameterType)
                {
                    case D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE:
                        variableType.mRootType = constant_shader_types::RootType::Table;
                        break;
                    case D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS:
                        variableType.mRootType = constant_shader_types::RootType::Constant;
                        break;
                    case D3D12_ROOT_PARAMETER_TYPE_CBV:
                        variableType.mRootType = constant_shader_types::RootType::ConstantBufferView;
                        break;
                    case D3D12_ROOT_PARAMETER_TYPE_SRV:
                        variableType.mRootType = constant_shader_types::RootType::ShaderResourceView;
                        break;
                    case D3D12_ROOT_PARAMETER_TYPE_UAV:
                        variableType.mRootType = constant_shader_types::RootType::UnorderedAccessView;
                        break;
                }

                result[value.mVariableName] = variableType;
            }
            else
            {
                YLOG_ERROR("COMP", std::format("Variable name '{}' not found in mappings. Skipping.", value.mVariableTypeName).c_str());
            }
        }

        return result;
    }


    //-------------------------------------------------------------------------------------------------
    std::string GetVariableTypeName(ID3D12ShaderReflectionConstantBuffer* shaderReflectionConstantBuffer, const char* variableName)
    {
        std::string result;

        if (auto variable = shaderReflectionConstantBuffer->GetVariableByName(variableName))
        {
            auto variableType = variable->GetType();
            D3D12_SHADER_TYPE_DESC shaderTypeDesc{};
            HRESULT hr = variableType->GetDesc(&shaderTypeDesc);
            yaget::error_handlers::ThrowOnError(hr, std::format("Could not get Variable Type Description from ShaderReflectionVariable. VariableName: {}", variableName));
            result = shaderTypeDesc.Name;
        }

        return result;
    }

    std::string GetTextureTypeName(D3D_SRV_DIMENSION dimension)
    {
        switch (dimension)
        {
            case D3D_SRV_DIMENSION_TEXTURE1D:
                return "Texture1d";
                break;
            case D3D_SRV_DIMENSION_TEXTURE1DARRAY:
                return "Texture1dArray";
                break;
            case D3D_SRV_DIMENSION_TEXTURE2D:
                return "Texture2d";
                break;
            case D3D_SRV_DIMENSION_TEXTURE2DARRAY:
                return "Texture2dArray";
                break;
            case D3D_SRV_DIMENSION_TEXTURE3D:
                return "Texture3d";
                break;
            case D3D_SRV_DIMENSION_TEXTURECUBE:
                return "TextureCube";
                break;
            case D3D_SRV_DIMENSION_TEXTURECUBEARRAY:
                return "TextureCubeArray";
                break;
            default:
                YLOG_ERROR("COMP", std::format("Unsupported TextureType: '{}'", magic_enum::enum_name(dimension)).c_str());
                return "";
        }
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
            .Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
                     D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
                     D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS |
                     D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS,
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

    const RootDescResult rootResult = MakeRootSignature(rootParameters);

    descriptionCallback(rootResult);
}


//-------------------------------------------------------------------------------------------------
void yaget::render::ResourceReflector::PopulateMappings(io::VirtualTransportSystem::Section fileName, io::VirtualTransportSystem& vts)
{
    PopulateMap(fileName, vts, ReflectorVariableMappings);
}


//-------------------------------------------------------------------------------------------------
void yaget::render::ResourceReflector::SaveMappings(io::VirtualTransportSystem::Section fileName, io::VirtualTransportSystem& vts)
{
    SaveMap(fileName, vts, ReflectorVariableMappings);
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
    error_handlers::ThrowOnError(hr, std::format("Could not execute compiler for shader with params: '{}'", conv::Combine(parameters, ", ")));

    ComPtr<IDxcBlobUtf8> errors;
    hr = result->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&errors), nullptr);
    error_handlers::ThrowOnError(hr, "Could not get output for shader from compiled results.");

    if (errors && errors->GetStringLength())
    {
        error_handlers::ThrowOnError(hr, std::format("Did not compile shader. {}", errors->GetStringPointer()));
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
    catch (const ex::bad_init& ex)
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
                error_handlers::ThrowOnError(hr, std::format("Could not get input Parameter Description from compiled vertex shader. Parameter Index: {}", parameterIndex));

                return signatureParameterDesc;
            };

            for (const uint32_t parameterIndex : std::views::iota(0u, shaderDesc.InputParameters))
            {
                auto signatureParameterDesc = paramDesc(parameterIndex, [&shaderReflection = mShaderReflection](UINT parameterIndex, D3D12_SIGNATURE_PARAMETER_DESC* desc)
                {
                    return shaderReflection->GetInputParameterDesc(parameterIndex, desc);
                });

                shaderInputs.push_back({ .mName = signatureParameterDesc.SemanticName, .mIndex = signatureParameterDesc.SemanticIndex });
            }

            for (const uint32_t parameterIndex : std::views::iota(0u, shaderDesc.OutputParameters))
            {
                auto signatureParameterDesc = paramDesc(parameterIndex, [&shaderReflection = mShaderReflection](UINT parameterIndex, D3D12_SIGNATURE_PARAMETER_DESC* desc)
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

    mShaderType = static_cast<D3D12_SHADER_VERSION_TYPE>((shaderDesc.Version & 0xFFFF0000) >> 16);
    mMajorVersion = (shaderDesc.Version & 0x000000F0) >> 4;
    mMinorVersion = (shaderDesc.Version & 0x0000000F);

    mShaderVisibility = GeneratePins(mShaderType, shaderDesc);

    //------------------------------------------------------------------
    // next let's go over bounded resources
    for (const uint32_t i : std::views::iota(0u, shaderDesc.BoundResources))
    {
        D3D12_SHADER_INPUT_BIND_DESC shaderInputBindDesc{};
        hr = mShaderReflection->GetResourceBindingDesc(i, &shaderInputBindDesc);
        error_handlers::ThrowOnError(hr, std::format("Could not get Resource Binding Description from compiled vertex shader. BoundResources Index: {}", i));

        switch (shaderInputBindDesc.Type)
        {
            case D3D_SIT_CBUFFER:
            {
                ID3D12ShaderReflectionConstantBuffer* shaderReflectionConstantBuffer = mShaderReflection->GetConstantBufferByIndex(i);

                D3D12_SHADER_BUFFER_DESC constantBufferDesc{};
                hr = shaderReflectionConstantBuffer->GetDesc(&constantBufferDesc);
                error_handlers::ThrowOnError(hr, std::format("Could not get Constant Buffer Description from compiled vertex shader. BoundResources Index: {}", i));

                std::string variableTypeName = GetVariableTypeName(shaderReflectionConstantBuffer, constantBufferDesc.Name);
                error_handlers::ThrowOnCheck(!variableTypeName.empty(), std::format("Could not get variable type name for constant buffer: '{}'", constantBufferDesc.Name));

                // NOTE(eg) we may want to consider having path for small (one matrix?) root const buffer
                D3D12_ROOT_PARAMETER1 rootParameter = {};

                if (constantBufferDesc.Variables * constantBufferDesc.Size < sizeof(float) * 4)
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

                rootParameters.push_back({ .mParameter = rootParameter, .mVariableName = shaderInputBindDesc.Name, .mVariableTypeName = variableTypeName });
            }
            break;
            case D3D_SIT_TEXTURE:
            {
                //ID3D12ShaderReflectionConstantBuffer* shaderReflectionConstantBuffer = mShaderReflection->GetConstantBufferByIndex(i);

                //D3D12_SHADER_BUFFER_DESC constantBufferDesc{};
                //hr = shaderReflectionConstantBuffer->GetDesc(&constantBufferDesc);
                //error_handlers::ThrowOnError(hr, std::format("Could not get Constant Buffer Description from compiled vertex shader. BoundResources Index: {}", i));

                //std::string variableTypeName = GetVariableTypeName(shaderReflectionConstantBuffer, constantBufferDesc.Name);
                //error_handlers::ThrowOnCheck(!variableTypeName.empty(), std::format("Could not get variable type name for constant buffer: '{}'", constantBufferDesc.Name));

                rootParameters.push_back({});
                auto& parameter = rootParameters.back();
                const CD3DX12_DESCRIPTOR_RANGE1 srvRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV,
                                                         1u,
                                                         shaderInputBindDesc.BindPoint,
                                                         shaderInputBindDesc.Space,
                                                         D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC);

                parameter.mDescriptorRangesScratchPad.push_back(srvRange);

                parameter.mParameter =
                {
                    .ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE,
                    .DescriptorTable
                    {
                        .NumDescriptorRanges = 1u,
                        .pDescriptorRanges = &parameter.mDescriptorRangesScratchPad.back(),
                    },
                    .ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL,
                };

                parameter.mVariableName = shaderInputBindDesc.Name;
                parameter.mVariableTypeName = GetTextureTypeName(shaderInputBindDesc.Dimension);
            }
            break;
            case D3D_SIT_SAMPLER:
            {
                rootParameters.push_back({});
                auto& parameter = rootParameters.back();
                const CD3DX12_DESCRIPTOR_RANGE1 srvRange(D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER,
                                                         1u,
                                                         shaderInputBindDesc.BindPoint,
                                                         shaderInputBindDesc.Space);

                parameter.mDescriptorRangesScratchPad.push_back(srvRange);
                parameter.mParameter =
                {
                    .ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE,
                    .DescriptorTable
                    {
                        .NumDescriptorRanges = 1u,
                        .pDescriptorRanges = &parameter.mDescriptorRangesScratchPad.back(),
                    },
                    .ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL,
                };

                parameter.mVariableName = shaderInputBindDesc.Name;
                parameter.mVariableTypeName = "Sampler";
            }
            break;
            default:
                YLOG_ERROR("COMP", std::format("Unsupported Bound Resources type: '{}'", magic_enum::enum_name(shaderInputBindDesc.Type)).c_str());
        }
    }
}
