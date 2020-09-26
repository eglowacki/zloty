#include "Resources/ConstantsResource.h"
#include "Device.h"
#include <d3dcompiler.h>
#include <wrl/client.h>

using namespace Microsoft::WRL;

namespace
{
    //--------------------------------------------------------------------------------------------------
    size_t AllignTo(size_t numToRound, int multiple)
    {
        if (multiple == 0)
            return numToRound;

        int remainder = numToRound % multiple;
        if (remainder == 0)
            return numToRound;

        return numToRound + multiple - remainder;
    }

} // namespace


//--------------------------------------------------------------------------------------------------
yaget::render::ConstantsResource::ConstantsResource(Device& device, std::shared_ptr<io::render::ShaderAsset> asset)
    : ResourceView(device, asset->mTag, std::type_index(typeid(ConstantsResource)))
{
    Device::ID3D11Device_t* hardwareDevice = mDevice.GetDevice();

    auto buffer = asset->mBuffer;

    ComPtr<ID3D11ShaderReflection> reflector;
    HRESULT hr = D3DReflect(buffer.first.get(), buffer.second, IID_ID3D11ShaderReflection, &reflector);
    YAGET_THROW_ON_RROR(hr, "Could not create Vertex Shader Reflection");

    // Get shader info
    D3D11_SHADER_DESC shaderDesc = {};
    hr = reflector->GetDesc(&shaderDesc);
    YAGET_THROW_ON_RROR(hr, "Could not get Vertex Shader Reflection description");

    for (uint32_t i = 0; i < shaderDesc.ConstantBuffers; ++i)
    {
        Buffer constantBuffer;
        ID3D11ShaderReflectionConstantBuffer* constantReflection = reflector->GetConstantBufferByIndex(i);

        D3D11_SHADER_BUFFER_DESC desc;
        hr = constantReflection->GetDesc(&desc);
        YAGET_THROW_ON_RROR(hr, "Could not get Constant Buffer Reflection description");

        D3D11_SHADER_INPUT_BIND_DESC bindDesc = {};
        hr = reflector->GetResourceBindingDescByName(desc.Name, &bindDesc);
        YAGET_THROW_ON_RROR(hr, "Could not get Constant Buffer bind description");

        std::string bufferName = desc.Name;
        uint32_t bufferSize = desc.Size;
        uint32_t registerIndex = bindDesc.BindPoint;

        constantBuffer.mData.resize(bufferSize, 0);
        constantBuffer.mRegisterNumber = registerIndex;

        for (uint32_t v = 0; v < desc.Variables; ++v)
        {
            ID3D11ShaderReflectionVariable* reflectionVariable = constantReflection->GetVariableByIndex(v);
            ID3D11ShaderReflectionType* reflectionType = reflectionVariable->GetType();

            D3D11_SHADER_TYPE_DESC typeDesc = {};
            hr = reflectionType->GetDesc(&typeDesc);
            YAGET_THROW_ON_RROR(hr, "Could not get variable type description");

            D3D11_SHADER_VARIABLE_DESC varDesc = {};
            hr = reflectionVariable->GetDesc(&varDesc);
            YAGET_THROW_ON_RROR(hr, "Could not get reflection variable description");

            std::string variableName = varDesc.Name;
            //uint32_t variableSize = varDesc.Size;
            uint32_t variableOffset = varDesc.StartOffset;
            std::string variableType = typeDesc.Name;

            constantBuffer.mSlots[variableName] = variableOffset;
        }

        D3D11_BUFFER_DESC constantBufferDesc = {};
        constantBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        constantBufferDesc.ByteWidth = static_cast<uint32_t>(constantBuffer.mData.size());
        constantBufferDesc.CPUAccessFlags = 0;
        constantBufferDesc.Usage = D3D11_USAGE_DEFAULT;

        hr = hardwareDevice->CreateBuffer(&constantBufferDesc, nullptr, &constantBuffer.mConstants);
        YAGET_THROW_ON_RROR(hr, "Could not create constants buffer");

        YAGET_SET_DEBUG_NAME(constantBuffer.mConstants.Get(), asset->mTag.mName);

        mBuffers[bufferName] = constantBuffer;
    }

    std::size_t hashValue = asset->mTag.Hash();
    SetHashValue(hashValue);
}


//--------------------------------------------------------------------------------------------------
bool yaget::render::ConstantsResource::Activate()
{
    mDevice.ActivatedResource(this, fmt::format("This: {}, Hash: {}", static_cast<void*>(this), GetStateHash()).c_str());

    Device::ID3D11DeviceContext_t* hardwareContext = mDevice.GetDeviceContext();

    for (auto& it : mBuffers)
    {
        
        if (it.second.mDirty)
        {
            it.second.mDirty = false;
            hardwareContext->UpdateSubresource(it.second.mConstants.Get(), 0, nullptr, it.second.mData.data(), 0, 0);
        }

        switch (mType)
        {
        case Type::Vertex:
            hardwareContext->VSSetConstantBuffers(it.second.mRegisterNumber, 1, it.second.mConstants.GetAddressOf());
            break;
        case Type::Pixel:
            hardwareContext->PSSetConstantBuffers(it.second.mRegisterNumber, 1, it.second.mConstants.GetAddressOf());
            break;
        }
    }

    return true;
}
