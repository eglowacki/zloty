#include "Render/RenderCore.h"
#include "Core/ErrorHandlers.h"
#include "Render/Pipeline/RenderSignatures.h"

#include <d3dx12.h>


namespace
{
    yaget::render::ComPtr<ID3D12RootSignature> CreateRootSignature(const yaget::io::Tag& tag, ID3D12Device* device, yaget::io::Buffer& buffer, const yaget::render::RenderShaders::RootDescResult& rootSignatureDesc)
    {
        using namespace yaget;

        char* bufferPointer = io::cast_data<char>(buffer);
        size_t bufferSize = io::size_data(buffer);

        if (!bufferSize)
        {
            render::ComPtr<ID3DBlob> error;

            //// https://asawicki.info/news_1754_direct3d_12_long_way_to_access_data
            render::ComPtr<ID3DBlob> signature;
            HRESULT hr = D3D12SerializeVersionedRootSignature(&rootSignatureDesc.mRootSignatureDesc, &signature, &error);
            error_handlers::ThrowOnError(hr, std::format("Could not serialize root signature: '{}'. Error: {}", conv::Convertor<io::Tag>::ToString(tag), error ? static_cast<const char*>(error->GetBufferPointer()) : ""));

            bufferPointer = static_cast<char*>(signature->GetBufferPointer());
            bufferSize = signature->GetBufferSize();
            buffer = io::CreateBuffer(bufferPointer, bufferSize);
        }

        render::ComPtr<ID3D12RootSignature> rootSignature;
        HRESULT hr = device->CreateRootSignature(0, bufferPointer, bufferSize, IID_PPV_ARGS(&rootSignature));
        error_handlers::ThrowOnError(hr, std::format("Could not create root signature: '{}'.", conv::Convertor<io::Tag>::ToString(tag)));
        YLOG_INFO("REND", "Created Root Signature for: '%s'", conv::Convertor<io::Tag>::ToString(tag).c_str());

        return rootSignature;
    }
}


//-------------------------------------------------------------------------------------------------
yaget::render::RenderSignatures::RenderSignatures(ID3D12Device* device, io::VirtualTransportSystem& vts, io::VirtualTransportSystem::Section fileName)
    : CacheWatcher(vts, fileName)
    , mDevice(device)
{
}


//-------------------------------------------------------------------------------------------------
yaget::render::RenderSignatures::~RenderSignatures() = default;


//-------------------------------------------------------------------------------------------------
ID3D12RootSignature* yaget::render::RenderSignatures::GetSignature(const io::Tag& tag, const RenderShaders::RootDescResult& rootDescResult)
{
    YAGET_ASSERT(tag.IsValid(), "Tag: '%s:%s' is not valid.", yaget::conv::Convertor<yaget::Guid>::ToString(tag.mGuid).c_str(), yaget::conv::Convertor<yaget::io::Tag>::ToString(tag).c_str());

    // NOTE(eg) We need to have a better way to use mutex here, less granularity
    std::lock_guard mutexLocker(mMutex);

    auto result = GetAsset(tag, [this, &rootDescResult](auto tag, auto& cachedData)
    {
        return CreateRootSignature(tag, mDevice, cachedData, rootDescResult);
    });

    if (!mShaderIndexMap.contains(tag))
    {
        mShaderIndexMap.insert({ tag, rootDescResult.mIndexMap });
    }
    return result.Get();
}


//-------------------------------------------------------------------------------------------------
ID3D12RootSignature* yaget::render::RenderSignatures::GetSignature(const io::Tag& tag)
{
    RenderShaders::RootDescResult rootDescResult;
    return GetSignature(tag, rootDescResult);
}


//-------------------------------------------------------------------------------------------------
const yaget::render::RenderShaders::IndexMap& yaget::render::RenderSignatures::GetIndexMap(const io::Tag& tag)
{
    static RenderShaders::IndexMap emptyMap;
    if (mShaderIndexMap.contains(tag))
    {
        return mShaderIndexMap[tag];
    }

    return emptyMap;
}


//-------------------------------------------------------------------------------------------------
void yaget::render::RenderSignatures::ClearCache(const io::Tag& tag)
{
    {
        std::lock_guard mutexLocker(mMutex);
        mShaderIndexMap.erase(tag);
    }

    CacheWatcher<ComPtr<ID3D12RootSignature>>::ClearCache(tag);
}


//-------------------------------------------------------------------------------------------------
void yaget::render::RenderSignatures::PopulateMappings(io::VirtualTransportSystem::Section /*fileName*/, io::VirtualTransportSystem& /*vts*/)
{
}


//-------------------------------------------------------------------------------------------------
void yaget::render::RenderSignatures::SaveMappings(io::VirtualTransportSystem::Section /*fileName*/, io::VirtualTransportSystem& /*vts*/)
{
}
