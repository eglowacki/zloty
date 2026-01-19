#include "Renders/RenderSignatures.h"
//#include <d3d12.h>
#include <d3dx12.h>
#include <Fmt/format.h>

#include "Core/ErrorHandlers.h"


namespace
{
   
    yaget::render::ComPtr<ID3D12RootSignature> CreateRootSignature(uint64_t /*sigType*/, ID3D12Device* device, yaget::io::Buffer& buffer)
    {
        yaget::render::ComPtr<ID3DBlob> signature;
        char* bufferPointer = yaget::io::cast_data<char>(buffer);
        size_t bufferSize = yaget::io::size_data(buffer);

        if (!yaget::io::size_data(buffer))
        {
            yaget::render::ComPtr<ID3DBlob> error;

            // https://asawicki.info/news_1754_direct3d_12_long_way_to_access_data
            D3D12_ROOT_PARAMETER1 parameters[1] = {};
            parameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
            parameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
            parameters[0].Constants.ShaderRegister = 3;
            parameters[0].Constants.RegisterSpace = 0;
            parameters[0].Constants.Num32BitValues = 16;
            CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc(1, static_cast<const D3D12_ROOT_PARAMETER1*>(parameters), 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

            HRESULT hr = ::D3D12SerializeVersionedRootSignature(&rootSignatureDesc, &signature, &error);
            yaget::error_handlers::ThrowOnError(hr, fmt::format("Could not serialize root signature. {}", error ? static_cast<const char*>(error->GetBufferPointer()) : ""));

            bufferPointer = static_cast<char*>(signature->GetBufferPointer());
            bufferSize = signature->GetBufferSize();
            buffer = yaget::io::CreateBuffer(bufferPointer, bufferSize);
        }

        yaget::render::ComPtr<ID3D12RootSignature> rootSignature;
        HRESULT hr = device->CreateRootSignature(0, bufferPointer, bufferSize, IID_PPV_ARGS(&rootSignature));
        yaget::error_handlers::ThrowOnError(hr, "Could not create root signature.");

        return rootSignature;
    }
}


//-------------------------------------------------------------------------------------------------
defensor::render::RenderSignatures::RenderSignatures(ID3D12Device* device, yaget::io::VirtualTransportSystem& vts)
    : mDevice(device)
    , mVTS(vts)
    , mCache(mVTS, yaget::io::VirtualTransportSystem::Section("Caches@Signatures"))
{
}


//-------------------------------------------------------------------------------------------------
defensor::render::RenderSignatures::~RenderSignatures() = default;


//-------------------------------------------------------------------------------------------------
ID3D12RootSignature* defensor::render::RenderSignatures::GetSignature(uint64_t sigType)
{
    YAGET_ASSERT(sigType < yaget::render::AssetCache::TypeToTag.size(), "sigType '%d' is not valid for signature", sigType);

    if (auto it = mSignatures.find(sigType); it != mSignatures.end())
    {
        return it->second.Get();
    }

    yaget::io::Buffer cachedData = mCache.GetCachedAsset(yaget::render::AssetCache::TypeToTag[sigType]);
    bool missingCachedData = yaget::io::size_data(cachedData) == 0;

    auto sig = CreateRootSignature(sigType, mDevice, cachedData);
    mSignatures.insert({sigType, sig});

    if (yaget::io::size_data(cachedData) && missingCachedData)
    {
        mCache.SaveCachedAsset(yaget::render::AssetCache::TypeToTag[sigType], cachedData);
    }

    return sig.Get();
}
