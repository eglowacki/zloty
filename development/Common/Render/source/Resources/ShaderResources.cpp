#include "Resources/ShaderResources.h"
#include "App/AppUtilities.h"
#include "Core/ErrorHandlers.h"
#include "Device.h"
#include "Json/JsonHelpers.h"
#include "Logger/YLog.h"
#include "RenderHelpers.h"
#include "Streams/Buffers.h"

#include <filesystem>
#include <d3dcompiler.h>
#include <VertexTypes.h>

using namespace yaget;
using namespace Microsoft::WRL;
using namespace DirectX;
namespace fs = std::filesystem;

namespace 
{
    //--------------------------------------------------------------------------------------------------
    yaget::io::Buffer CompileSource(const char* sourceCode)
    {
        uint32_t flags = D3DCOMPILE_ENABLE_STRICTNESS;
#ifdef YAGET_DEBUG
        flags |= D3DCOMPILE_DEBUG;
#endif
        const D3D_SHADER_MACRO defines[] =
        {
            NULL, NULL
        };

        ComPtr<ID3DBlob> compiledCode;
        ComPtr<ID3DBlob> error;

        HRESULT hr = D3DCompile(sourceCode, std::strlen(sourceCode), nullptr, defines, nullptr, "main", "vs_5_0", flags, 0, &compiledCode, &error);
        if (FAILED(hr))
        {
            YLOG_ERROR("MAT", "Errors compiling shader: %s", reinterpret_cast<const char*>(error->GetBufferPointer()));
            return yaget::io::Buffer();
        }

        yaget::io::Buffer buffer = yaget::io::CreateBuffer(reinterpret_cast<const char*>(compiledCode->GetBufferPointer()), compiledCode->GetBufferSize());
        return buffer;
    }

} // namespace


//--------------------------------------------------------------------------------------------------
yaget::render::VertexShaderResource::VertexShaderResource(Device& device, std::shared_ptr<io::render::ShaderAsset> asset)
    : ResourceView(device, asset->mTag, std::type_index(typeid(VertexShaderResource)))
{
    Device::ID3D11Device_t* hardwareDevice = mDevice.GetDevice();

    auto buffer = asset->mBuffer;
    const HRESULT hr = hardwareDevice->CreateVertexShader(buffer.first.get(), buffer.second, nullptr, &mVertexShader);
    error_handlers::ThrowOnError(hr, "Could not create Vertex Shader");

    YAGET_SET_DEBUG_NAME(mVertexShader.Get(), asset->mTag.mName);

    std::size_t hashValue = asset->mTag.Hash();
    SetHashValue(hashValue);
    SetPlatformResource(mVertexShader.Get());
}


//--------------------------------------------------------------------------------------------------
bool yaget::render::VertexShaderResource::Activate()
{
    mDevice.ActivatedResource(this, fmt::format("This: {}, Hash: {}", static_cast<void*>(this), GetStateHash()).c_str());

    Device::ID3D11DeviceContext_t* deviceContext = mDevice.GetDeviceContext();

    deviceContext->VSSetShader(mVertexShader.Get(), nullptr, 0);
    return true;
}


//--------------------------------------------------------------------------------------------------
yaget::render::PixelShaderResource::PixelShaderResource(Device& device, std::shared_ptr<io::render::ShaderAsset> asset)
    : ResourceView(device, asset->mTag, std::type_index(typeid(PixelShaderResource)))
{
    Device::ID3D11Device_t* hardwareDevice = mDevice.GetDevice();

    const auto buffer = asset->mBuffer;
    const HRESULT hr = hardwareDevice->CreatePixelShader(buffer.first.get(), buffer.second, nullptr, &mPixelShader);
    error_handlers::ThrowOnError(hr, "Could not create Pixel Shader");

    YAGET_SET_DEBUG_NAME(mPixelShader.Get(), asset->mTag.mName);

    std::size_t hashValue = asset->mTag.Hash();
    SetHashValue(hashValue);
    SetPlatformResource(mPixelShader.Get());
}


//--------------------------------------------------------------------------------------------------
bool yaget::render::PixelShaderResource::Activate()
{
    mDevice.ActivatedResource(this, fmt::format("This: {}, Hash: {}", static_cast<void*>(this), GetStateHash()).c_str());

    Device::ID3D11DeviceContext_t* deviceContext = mDevice.GetDeviceContext();

    deviceContext->PSSetShader(mPixelShader.Get(), nullptr, 0);
    return true;
}


//--------------------------------------------------------------------------------------------------
yaget::render::InputLayoutResource::InputLayoutResource(Device& device, std::shared_ptr<io::render::ShaderAsset> asset)
    : ResourceView(device, asset->mTag, std::type_index(typeid(InputLayoutResource)))
{
    auto buffer = asset->mBuffer;

    ComPtr<ID3D11ShaderReflection> reflector;
    HRESULT hr = D3DReflect(buffer.first.get(), buffer.second, IID_ID3D11ShaderReflection, &reflector);
    error_handlers::ThrowOnError(hr, "Could not create Vertex Shader Reflection");

    // Get shader info
    D3D11_SHADER_DESC shaderDesc;
    reflector->GetDesc(&shaderDesc);

    // Read input layout description from shader info
    std::vector<D3D11_INPUT_ELEMENT_DESC> elements;
    for (uint32_t i = 0; i < shaderDesc.InputParameters; ++i)
    {
        D3D11_SIGNATURE_PARAMETER_DESC paramDesc = {};
        hr = reflector->GetInputParameterDesc(i, &paramDesc);
        error_handlers::ThrowOnError(hr, "Could not get Input Parameter Desc");

        // fill out input element desc
        D3D11_INPUT_ELEMENT_DESC elementDesc = {};
        elementDesc.SemanticName = paramDesc.SemanticName;
        elementDesc.SemanticIndex = paramDesc.SemanticIndex;
        elementDesc.InputSlot = i;
        elementDesc.AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
        elementDesc.InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
        elementDesc.InstanceDataStepRate = 0;

        // determine DXGI format
        if (paramDesc.Mask == 1)
        {
            if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_UINT32) elementDesc.Format = DXGI_FORMAT_R32_UINT;
            else if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_SINT32) elementDesc.Format = DXGI_FORMAT_R32_SINT;
            else if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32) elementDesc.Format = DXGI_FORMAT_R32_FLOAT;
        }
        else if (paramDesc.Mask <= 3)
        {
            if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_UINT32) elementDesc.Format = DXGI_FORMAT_R32G32_UINT;
            else if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_SINT32) elementDesc.Format = DXGI_FORMAT_R32G32_SINT;
            else if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32) elementDesc.Format = DXGI_FORMAT_R32G32_FLOAT;
        }
        else if (paramDesc.Mask <= 7)
        {
            if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_UINT32) elementDesc.Format = DXGI_FORMAT_R32G32B32_UINT;
            else if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_SINT32) elementDesc.Format = DXGI_FORMAT_R32G32B32_SINT;
            else if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32) elementDesc.Format = DXGI_FORMAT_R32G32B32_FLOAT;
        }
        else if (paramDesc.Mask <= 15)
        {
            if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_UINT32) elementDesc.Format = DXGI_FORMAT_R32G32B32A32_UINT;
            else if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_SINT32) elementDesc.Format = DXGI_FORMAT_R32G32B32A32_SINT;
            else if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32) elementDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
        }

        //save element desc
        elements.push_back(elementDesc);
    }

    Device::ID3D11Device_t* hardwareDevice = mDevice.GetDevice();

    hr = hardwareDevice->CreateInputLayout(elements.data(), static_cast<uint32_t>(elements.size()), buffer.first.get(), static_cast<uint32_t>(buffer.second), &mInputLayout);
    error_handlers::ThrowOnError(hr, "Could not create InputLayout.");

    YAGET_SET_DEBUG_NAME(mInputLayout.Get(), asset->mTag.mName);

    std::size_t hashValue = asset->mTag.Hash();
    SetHashValue(hashValue);
    SetPlatformResource(mInputLayout.Get());
}


//--------------------------------------------------------------------------------------------------
bool yaget::render::InputLayoutResource::Activate()
{
    mDevice.ActivatedResource(this, fmt::format("This: {}, Hash: {}", static_cast<void*>(this), GetStateHash()).c_str());

    Device::ID3D11DeviceContext_t* deviceContext = mDevice.GetDeviceContext();

    deviceContext->IASetInputLayout(mInputLayout.Get());
    return true;
}
