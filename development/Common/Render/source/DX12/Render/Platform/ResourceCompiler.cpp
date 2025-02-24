#include "Render/Platform/ResourceCompiler.h"
#include "App/AppUtilities.h"
#include "Fmt/format.h"

#include <d3dx12.h>
#include <d3dcompiler.h>

#include <dxcapi.h>         // Be sure to link with dxcompiler.lib.
#include <d3d12shader.h>    // Shader reflection.

#include "Core/ErrorHandlers.h"

// New compiler for shaders
// https://github.com/microsoft/DirectXShaderCompiler/wiki/Using-dxc.exe-and-dxcompiler.dll



//-------------------------------------------------------------------------------------------------
yaget::render::ResourceCompiler::ResourceCompiler(io::BufferView data, const char* entryName, const char* target, bool useNewestCompiler)
{
    if (useNewestCompiler)
    {
        ComPtr<IDxcUtils> utils;
        HRESULT hr = DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&utils));
        error_handlers::ThrowOnError(hr, "Could not create instance of IDxcUtils");

        ComPtr<IDxcCompiler3> compiler;
        hr = DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&compiler));
        error_handlers::ThrowOnError(hr, "Could not create instance of IDxcCompiler3");

        ComPtr<IDxcIncludeHandler> pIncludeHandler;
        hr = utils->CreateDefaultIncludeHandler(&pIncludeHandler);
        error_handlers::ThrowOnError(hr, "Could not create Default Include Handler");
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
        const HRESULT hr = ::D3DCompile(data.first, data.second, nullptr, nullptr, nullptr, entryName, target, compileFlags, 0, &mShaderBlob, &error);
        error_handlers::ThrowOnError(hr, fmt::format("Could not compile shader with entry point: '{}' and target: '{}'. {}", entryName, target, (error ? static_cast<const char*>(error->GetBufferPointer()) : "")));
    }
}


//-------------------------------------------------------------------------------------------------
ID3D10Blob* yaget::render::ResourceCompiler::GetCompiled() const
{
    return mShaderBlob.Get();
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

    YLOG_NOTICE("COMP", "Shader Reflection: Input Parameters = %d, Output Parameters = %d.", desc.InputParameters, desc.OutputParameters);

    for (uint32_t i = 0; i < desc.InputParameters; ++i)
    {
        D3D12_SIGNATURE_PARAMETER_DESC paramDesc = {};
        hr = shaderReflectior->GetInputParameterDesc(i, &paramDesc);
        error_handlers::ThrowOnError(hr, "Could not get input param description for shader data");

        ID3D12ShaderReflectionVariable* shaderVariable = shaderReflectior->GetVariableByName(paramDesc.SemanticName);

        D3D12_SHADER_VARIABLE_DESC varDesc = {};
        hr = shaderVariable->GetDesc(&varDesc);
        //error_handlers::ThrowOnError(hr, "Could not get input shaderVariable for shader data");


        YLOG_NOTICE("COMP", "Shader Reflection: Input Parameter Index: %d, Name: '%s', SemanticIndex: %d, Register: %d.", i, paramDesc.SemanticName, paramDesc.SemanticIndex, paramDesc.Register);
    }

    for (uint32_t i = 0; i < desc.OutputParameters; ++i)
    {
        D3D12_SIGNATURE_PARAMETER_DESC paramDesc = {};
        hr = shaderReflectior->GetOutputParameterDesc(i, &paramDesc);
        error_handlers::ThrowOnError(hr, "Could not get output param description for shader data");

        YLOG_NOTICE("COMP", "Shader Reflection: Output Parameter Index: %d, Name: '%s', SemanticIndex: %d, Register: %d.", i, paramDesc.SemanticName, paramDesc.SemanticIndex, paramDesc.Register);
    }
}
