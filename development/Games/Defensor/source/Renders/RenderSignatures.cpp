#include "Renders/RenderSignatures.h"
//#include <d3d12.h>
#include <d3dx12.h>
#include <Fmt/format.h>

#include "Core/ErrorHandlers.h"


namespace
{
   
    yaget::render::ComPtr<ID3D12RootSignature> CreateRootSignature(ID3D12Device* device)
    {
        CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
        rootSignatureDesc.Init(0, nullptr, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

        yaget::render::ComPtr<ID3DBlob> signature;
        yaget::render::ComPtr<ID3DBlob> error;
        HRESULT hr = ::D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error);
        yaget::error_handlers::ThrowOnError(hr, fmt::format("Could not serialize root signature. {}", error ? static_cast<const char*>(error->GetBufferPointer()) : ""));

        yaget::render::ComPtr<ID3D12RootSignature> rootSignature;
        hr = device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&rootSignature));
        yaget::error_handlers::ThrowOnError(hr, "Could not create root signature.");

        return rootSignature;
    }
}


defensor::render::RenderSignatures::RenderSignatures(ID3D12Device* device)
    : mDevice(device)
{
}


defensor::render::RenderSignatures::~RenderSignatures() = default;


ID3D12RootSignature* defensor::render::RenderSignatures::GetSignature(uint64_t sigType)
{
    if (auto it = mSignatures.find(sigType); it != mSignatures.end())
    {
        return it->second.Get();
    }

    auto sig = CreateRootSignature(mDevice);
    mSignatures.insert({sigType, sig});
    return sig.Get();
}
