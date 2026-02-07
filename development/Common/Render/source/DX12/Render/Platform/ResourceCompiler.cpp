#include "Render/Platform/ResourceCompiler.h"
#include "App/AppUtilities.h"
#include "Fmt/format.h"

#include <d3dx12.h>
#include <d3dcompiler.h>

#include <dxcapi.h>         // Be sure to link with dxcompiler.lib.
#include <d3d12shader.h>    // Shader reflection.
#include <ranges>

#include "StringHelpers.h"
#include "Core/ErrorHandlers.h"
#include "Platform/Support.h"

// New compiler for shaders
// https://github.com/microsoft/DirectXShaderCompiler/wiki/Using-dxc.exe-and-dxcompiler.dll



//-------------------------------------------------------------------------------------------------
yaget::render::ResourceCompiler::ResourceCompiler(io::BufferView data, const char* entryName, const char* target, bool useOldCompiler, bool debugShaders)
{
    if (useOldCompiler == false)
    {
        ComPtr<IDxcUtils> utils;
        HRESULT hr = DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&utils));
        if(FAILED(hr))
        {
            YLOG_ERROR("COMP", fmt::format("Could not create DxcUtil object. {}", yaget::platform::LastErrorMessage()).c_str());
            return;
        }

        ComPtr<IDxcCompiler3> compiler;
        hr = DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&compiler));
        if(FAILED(hr))
        {
            YLOG_ERROR("COMP", fmt::format("Could not create DxcCompiler3 object. {}", yaget::platform::LastErrorMessage()).c_str());
            return;
        }

        std::vector<std::wstring> arguments;
        arguments.push_back(L"-E");
        arguments.push_back(conv::utf8_to_wide(entryName));

        // -T for the target profile (eg. 'ps_6_6')
        arguments.push_back(L"-T");
        arguments.push_back(conv::utf8_to_wide(target));

        arguments.push_back(L"-encoding");
        arguments.push_back(L"utf8");

        if (debugShaders)
        {
            arguments.push_back(L"-Zi");
            arguments.push_back(L"-Qembed_debug");
            arguments.push_back(L"-Od");
        }

        arguments.push_back(L"-fdiagnostics-format=msvc");

        //-rootsig-define <value> Read root signature from a #define
        
        std::vector<LPCWSTR> shaderArguments = arguments | std::views::transform([](const std::wstring& element)
        {
            return element.c_str();
        }) | std::ranges::to<std::vector<LPCWSTR>>();

        uint32_t codePage = CP_UTF8;
        DxcText dxBuffer{ io::cast_data<const char>(data), io::size_data(data), codePage };
        ComPtr<IDxcResult> result;
        hr = compiler->Compile(&dxBuffer, shaderArguments.data(), static_cast<UINT32>(shaderArguments.size()), nullptr, IID_PPV_ARGS(&result));
        if(FAILED(hr))
        {
            YLOG_ERROR("COMP", "Could not execute compiler for shader. %s", yaget::platform::LastErrorMessage().c_str());
            return;
        }

        ComPtr<IDxcBlobUtf8> errors;
        hr = result->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&errors), nullptr);
        if(FAILED(hr))
        {
            YLOG_ERROR("COMP", "Could not get output for shader from compiled results. %s", yaget::platform::LastErrorMessage().c_str());
            return;
        }

        if (errors && errors->GetStringLength())
        {
            YLOG_ERROR("COMP", fmt::format("Could not compile shader. {}", errors->GetStringPointer()).c_str());
            return;
        }

        ComPtr<IDxcBlob> shaderBin;
        hr = result->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&shaderBin), nullptr);
        if(FAILED(hr))
        {
            YLOG_ERROR("COMP", "Could not get bin shader from compiled results");
            return;
        }

#if 0
        ComPtr<IDxcBlob> reflectionBlob{};
        hr = result->GetOutput(DXC_OUT_REFLECTION, IID_PPV_ARGS(&reflectionBlob), nullptr);
        if (SUCCEEDED(hr))
        {
            const DxcBuffer reflectionBuffer
            {
                .Ptr = reflectionBlob->GetBufferPointer(),
                .Size = reflectionBlob->GetBufferSize(),
                .Encoding = 0,
            };

            ComPtr<ID3D12ShaderReflection> shaderReflection{};
            hr = utils->CreateReflection(&reflectionBuffer, IID_PPV_ARGS(&shaderReflection));
            if (SUCCEEDED(hr))
            {
                D3D12_SHADER_DESC shaderDesc{};
                hr = shaderReflection->GetDesc(&shaderDesc);        
                if (SUCCEEDED(hr))
                {
                    auto shaderType = (shaderDesc.Version & 0xFFFF0000) >> 16;
                    if (shaderType == D3D12_SHVER_VERTEX_SHADER)
                    {
                        //inputElementSemanticNames.reserve(shaderDesc.InputParameters);
                        //inputElementDescs.reserve(shaderDesc.InputParameters);
                        auto numInputParameters = shaderDesc.InputParameters;
                        std::vector<D3D12_INPUT_ELEMENT_DESC> inputElementDescs;

                        for (const uint32_t parameterIndex : std::views::iota(0u, numInputParameters))
                        {
                            D3D12_SIGNATURE_PARAMETER_DESC signatureParameterDesc{};
                            hr = shaderReflection->GetInputParameterDesc(parameterIndex, &signatureParameterDesc);
                            //if (SUCCEEDED(hr))

                            // Using the semantic name provided by the signatureParameterDesc directly to the input element desc will cause the SemanticName field to have garbage values.
                            // This is because the SemanticName filed is a const wchar_t*. I am using a separate std::vector<std::string> for simplicity.
                            std::string semanticName = signatureParameterDesc.SemanticName;
                            //inputElementSemanticNames.emplace_back(signatureParameterDesc.SemanticName);

                            inputElementDescs.emplace_back(D3D12_INPUT_ELEMENT_DESC{
                                        .SemanticName = semanticName.c_str(),
                                        .SemanticIndex = signatureParameterDesc.SemanticIndex,
                                        //.Format = maskToFormat(signatureParameterDesc.Mask),
                                        .InputSlot = 0u,
                                        .AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT,
                                        .InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
                                        // There doesn't seem to be a obvious way to 
                                        // automate this currently, which might be a issue when instanced rendering is used
                                        .InstanceDataStepRate = 0u,
                                });
                        }

                        //inputLayoutDesc =
                        //{
                        //    .pInputElementDescs = inputElementDescs.data(),
                        //    .NumElements = static_cast<uint32_t>(inputElementDescs.size()),
                        //};
                    }
                }

                int z = 0;
                z;
            }

            int z = 0;
            z;
        }
#endif // 0

        const auto bufferSize = shaderBin->GetBufferSize();
        mBinaryBlob = io::CreateBuffer(static_cast<const char*>(shaderBin->GetBufferPointer()), bufferSize);

        //ResourceReflector resourceReflector(io::cast_to_view(mBinaryBlob));
    }
    else
    {
#if YAGET_DEBUG_RENDER == 1
        // Enable better shader debugging with the graphics debugging tools.
        UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else // YAGET_DEBUG_RENDER == 1
        UINT compileFlags = 0;
#endif // YAGET_DEBUG_RENDER == 1

        ComPtr<ID3DBlob> error;
        ComPtr<ID3DBlob> shaderBlob;
        const HRESULT hr = ::D3DCompile(io::BufferPointer(data), io::BufferSize(data), nullptr, nullptr, nullptr, entryName, target, compileFlags, 0, &shaderBlob, &error);

        YLOG_CERROR("COMP", SUCCEEDED(hr), fmt::format("Could not compile shader with entry point: '{}' and target: '{}'.\n{}", entryName, target, (error ? static_cast<const char*>(error->GetBufferPointer()) : "")).c_str());
        if (SUCCEEDED(hr))
        {
            mBinaryBlob = io::CreateBuffer(static_cast<const char*>(shaderBlob->GetBufferPointer()), shaderBlob->GetBufferSize());
        }
    }
}


//-------------------------------------------------------------------------------------------------
yaget::io::Buffer yaget::render::ResourceCompiler::GetCompiled() const
{
    return mBinaryBlob;
}


//-------------------------------------------------------------------------------------------------
yaget::render::ResourceReflector::ResourceReflector(io::BufferView data)
{
    //ComPtr<ID3D12LibraryReflection> reflector;
    //HRESULT hr = ::D3DReflectLibrary(data, size, IID_ID3D12LibraryReflection, &reflector);
    //error_handlers::ThrowOnError(hr, "Could not get library reflection object for shader data");

    //D3D12_LIBRARY_DESC desc = {};
    //hr = mReflector->GetDesc(&desc);
    //error_handlers::ThrowOnError(hr, "Could not get library reflection description for shader data");

    //IID_ID3D12ShaderReflection
    ComPtr<ID3D12ShaderReflection> shaderReflectior;
    HRESULT hr = ::D3DReflect(data.first, data.second, IID_ID3D12ShaderReflection, &shaderReflectior);
    error_handlers::ThrowOnError(hr, "Could not get shader reflection object for shader data");

    //hr = shaderReflectior->QueryInterface(IID_ID3D12LibraryReflection, &reflector);

    D3D12_SHADER_DESC desc = {};
    hr = shaderReflectior->GetDesc(&desc);
    error_handlers::ThrowOnError(hr, "Could not get shader reflection description for shader data");

    YLOG_INFO("COMP", "Shader Reflection: Input Parameters = %d, Output Parameters = %d.", desc.InputParameters, desc.OutputParameters);

    for (uint32_t i = 0; i < desc.InputParameters; ++i)
    {
        D3D12_SIGNATURE_PARAMETER_DESC paramDesc = {};
        hr = shaderReflectior->GetInputParameterDesc(i, &paramDesc);
        error_handlers::ThrowOnError(hr, "Could not get input param description for shader data");

        ID3D12ShaderReflectionVariable* shaderVariable = shaderReflectior->GetVariableByName(paramDesc.SemanticName);

        D3D12_SHADER_VARIABLE_DESC varDesc = {};
        hr = shaderVariable->GetDesc(&varDesc);
        //error_handlers::ThrowOnError(hr, "Could not get input shaderVariable for shader data");


        YLOG_INFO("COMP", "Shader Reflection: Input Parameter Index: %d, Name: '%s', SemanticIndex: %d, Register: %d.", i, paramDesc.SemanticName, paramDesc.SemanticIndex, paramDesc.Register);
    }

    for (uint32_t i = 0; i < desc.OutputParameters; ++i)
    {
        D3D12_SIGNATURE_PARAMETER_DESC paramDesc = {};
        hr = shaderReflectior->GetOutputParameterDesc(i, &paramDesc);
        error_handlers::ThrowOnError(hr, "Could not get output param description for shader data");

        YLOG_INFO("COMP", "Shader Reflection: Output Parameter Index: %d, Name: '%s', SemanticIndex: %d, Register: %d.", i, paramDesc.SemanticName, paramDesc.SemanticIndex, paramDesc.Register);
    }
}
