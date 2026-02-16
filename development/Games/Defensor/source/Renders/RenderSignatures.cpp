#include "Renders/RenderSignatures.h"
//#include <d3d12.h>
#include <d3dx12.h>
#include <Fmt/format.h>

#include "Core/ErrorHandlers.h"
#include "Parsers/DependencyGraph.h"


namespace
{
   
    yaget::render::ComPtr<ID3D12RootSignature> CreateRootSignature(const yaget::io::Tag& /*tag*/, ID3D12Device* device, yaget::io::Buffer& buffer, const yaget::render::RenderShaders::RootDescResult& /*rootDescResult*/)
    {
        char* bufferPointer = yaget::io::cast_data<char>(buffer);
        size_t bufferSize = yaget::io::size_data(buffer);

        if (!bufferSize)
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

            yaget::render::ComPtr<ID3DBlob> signature;
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
defensor::render::RenderSignatures::RenderSignatures(ID3D12Device* device, yaget::io::VirtualTransportSystem& vts, yaget::DependencyGraph& dependencyGraph, io::Watcher& watcher)
    : CacheWatcher(vts, yaget::io::VirtualTransportSystem::Section("Caches@Signatures"), dependencyGraph, watcher)
    , mDevice(device)
{
}


//-------------------------------------------------------------------------------------------------
defensor::render::RenderSignatures::~RenderSignatures() = default;


//-------------------------------------------------------------------------------------------------
ID3D12RootSignature* defensor::render::RenderSignatures::GetSignature(const yaget::io::Tag& tag, const yaget::render::RenderShaders::RootDescResult& rootDescResult)
{
    YAGET_ASSERT(tag.IsValid(), "Tag: '%s:%s' is not valid.", yaget::conv::Convertor<yaget::Guid>::ToString(tag.mGuid).c_str(), yaget::conv::Convertor<yaget::io::Tag>::ToString(tag).c_str());

    // NOTE(eg) We need to have a better way to use mutex here, less granularity, possibly add array option to get signatures
    std::lock_guard mutexLocker(mMutex);

    auto result = GetAsset(tag, [this, &rootDescResult](auto tag, auto& cachedData)
    {
        return CreateRootSignature(tag, mDevice, cachedData, rootDescResult);
    });

    return result.Get();
}


//-------------------------------------------------------------------------------------------------
ID3D12RootSignature* defensor::render::RenderSignatures::GetSignature(const yaget::io::Tag& tag)
{
    yaget::render::RenderShaders::RootDescResult rootDescResult;
    return GetSignature(tag, rootDescResult);
}
