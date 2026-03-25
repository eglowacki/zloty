#include "Core/ErrorHandlers.h"
#include "Json/JsonHelpers.h"
#include "Render/Device.h"
#include "Render/Pipeline/RenderTextures.h"
#include "Render/Platform/Adapter.h"
#include "Render/Platform/D3D12MemAlloc.h"
#include "Streams/Guid.h"
#include "VTS/ResolvedAssets.h"

#include <d3dx12.h>
#include <d3dx12_resource_helpers.h>


//-------------------------------------------------------------------------------------------------
yaget::render::RenderTextures::RenderTextures(io::VirtualTransportSystem& vts, io::VirtualTransportSystem::Section fileName)
    : CacheWatcher(vts, fileName)
{
}


//-------------------------------------------------------------------------------------------------
yaget::render::RenderTextures::~RenderTextures() = default;


//-------------------------------------------------------------------------------------------------
yaget::io::Buffer yaget::render::RenderTextures::GetTexture(const io::Tag& tag)
{
    YAGET_ASSERT(tag.IsValid(), "Tag: '%s:%s' is not valid.",
                 yaget::conv::ToString(tag.mGuid).c_str(),
                 yaget::conv::ToString(tag).c_str());

    auto result = GetTextures(io::Tags{ tag });
    return !result.empty() ? *result.begin() : io::Buffer{};
}


//-------------------------------------------------------------------------------------------------
std::vector<yaget::io::Buffer> yaget::render::RenderTextures::GetTextures(const io::Tags& tags)
{
    std::vector<io::Buffer> results;

    std::lock_guard mutexLocker(mMutex);

    for (auto tag : tags)
    {
        auto result = GetAsset(tag, [this, &results](auto tag, auto& cachedData)
        {
            if (!io::size_data(cachedData))
            {
                io::SingleBLobLoader<io::TextureAsset> loader(mVTS, tag);
                auto textureAsset = loader.GetAsset();
                cachedData = textureAsset && textureAsset->IsValid() ? textureAsset->mBuffer : io::Buffer{};
            }

            return cachedData;
        });

        results.push_back(result);
    }

    return results;
}


//-------------------------------------------------------------------------------------------------
void yaget::render::RenderTextures::PopulateMappings(io::VirtualTransportSystem::Section /*fileName*/, io::VirtualTransportSystem& /*vts*/)
{
    //PopulateMap(fileName, vts, ShaderOptionsMappings);
}


//-------------------------------------------------------------------------------------------------
void yaget::render::RenderTextures::SaveMappings(io::VirtualTransportSystem::Section /*fileName*/, io::VirtualTransportSystem& /*vts*/)
{
    //SaveMap(fileName, vts, ShaderOptionsMappings);
}


//-------------------------------------------------------------------------------------------------
yaget::render::TextureResources::TextureResources(DeviceB& device, RenderTextures& renderTextures)
    : mDevice(device)
      , mRenderTextures(renderTextures)
{
}


//-------------------------------------------------------------------------------------------------
yaget::render::TextureResources::~TextureResources() = default;


//-------------------------------------------------------------------------------------------------
std::vector<yaget::render::ComPtr<ID3D12Resource>> yaget::render::TextureResources::GetResources(const io::Tags& /*tags*/)
{
    std::vector<ComPtr<ID3D12Resource>> results;

#if 0
    for (const auto& tag : tags)
    {
        {
            mt::ReadLock readLocker(mSharedMutex);
            if (auto it = mResources.find(tag); it != mResources.end())
            {
                results.push_back(it->second.mResource);
            }
        }

        auto textureBuffer = mRenderTextures.GetTexture(tag);
        if (!io::size_data(textureBuffer))
        {
            YLOG_ERROR("REND", "Could not find texture data for tag: '%s'.", yaget::conv::ToString(tag).c_str());
            continue;
        }

        mt::WriteLock writeLocker(mSharedMutex);
        auto [textureHeader, texturePixels] = io::TextureAsset::ParseBuffer(textureBuffer);
        if (textureHeader.GetImageSize())
        {
            if (textureHeader.mComponents == 3)
            {
                YLOG_ERROR("REND", "There is no support for 24 bit images. Tag: '%s'", yaget::conv::ToString(tag).c_str());
                continue;
            }

            DXGI_FORMAT textureFormat = DXGI_FORMAT_UNKNOWN;
            switch (textureHeader.mComponents)
            {
                case 1:
                    textureFormat = DXGI_FORMAT_R8_UNORM;
                    break;
                case 2:
                    textureFormat = DXGI_FORMAT_R8G8_UNORM;
                    break;
                case 4:
                    textureFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
                    break;
                default:
                    YLOG_ERROR("REND", "There is no support for '%d' bit images. Tag: '%s'", textureHeader.GetPixelBits(), yaget::conv::ToString(tag).c_str());
            }


            D3D12_RESOURCE_DESC textureDesc = {};
            textureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
            textureDesc.Alignment = 0;
            textureDesc.Width = textureHeader.mSizeX;
            textureDesc.Height = textureHeader.mSizeY;
            textureDesc.DepthOrArraySize = 1;
            textureDesc.MipLevels = 1;
            textureDesc.Format = textureFormat;
            textureDesc.SampleDesc.Count = 1;
            textureDesc.SampleDesc.Quality = 0;
            textureDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
            textureDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

            D3D12MA::ALLOCATION_DESC heapDesc = {};
            heapDesc.HeapType = D3D12_HEAP_TYPE_DEFAULT;
            heapDesc.ExtraHeapFlags = D3D12_HEAP_FLAG_NONE;

            //const auto& adapter = mDevice.GetAdapter();
            auto allocator = mDevice.GetAdapter().GetAllocator();
            D3D12MA::Allocation* allocation = nullptr;;
            ComPtr<ID3D12Resource> texture;

            HRESULT hr = allocator->CreateResource(
                &heapDesc,
                &textureDesc,
                D3D12_RESOURCE_STATE_COPY_DEST,
                nullptr,
                &allocation,
                IID_PPV_ARGS(&texture));
            error_handlers::ThrowOnError(hr, std::format("Could not allocate texture resource for tag: {}", conv::ToString(tag)));

            const auto uploadBufferSize = GetRequiredIntermediateSize(texture.Get(), 0, 1);
            //const auto uploadBufferSize = GetRequiredIntermediateSize(texture.Get(), mAdapter.GetDevice());

            D3D12_RESOURCE_DESC uploadDesc = {};
            uploadDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
            uploadDesc.Alignment = 0;
            uploadDesc.Width = uploadBufferSize;
            uploadDesc.Height = 1;
            uploadDesc.DepthOrArraySize = 1;
            uploadDesc.MipLevels = 1;
            uploadDesc.Format = DXGI_FORMAT_UNKNOWN;
            uploadDesc.SampleDesc.Count = 1;
            uploadDesc.SampleDesc.Quality = 0;
            uploadDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
            uploadDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

            D3D12MA::ALLOCATION_DESC uploadHeapDesc = {};
            uploadHeapDesc.HeapType = D3D12_HEAP_TYPE_UPLOAD;
            uploadHeapDesc.ExtraHeapFlags = D3D12_HEAP_FLAG_NONE;

            D3D12MA::Allocation* uploadAllocation = nullptr;;
            ComPtr<ID3D12Resource> textureUploadHeap;

            hr = allocator->CreateResource(
                &uploadHeapDesc,
                &uploadDesc,
                D3D12_RESOURCE_STATE_GENERIC_READ,
                nullptr,
                &uploadAllocation,
                IID_PPV_ARGS(&textureUploadHeap));
            error_handlers::ThrowOnError(hr, std::format("Could not allocate upload texture resource for tag: '{}', size: '{}'", conv::ToString(tag), uploadBufferSize));

            D3D12_SUBRESOURCE_DATA textureData = {};
            textureData.pData = io::cast_data<uint8_t>(texturePixels);
            textureData.RowPitch = textureHeader.GetStride();
            textureData.SlicePitch = textureHeader.GetImageSize();

            auto framerHandler = mDevice.GetFramerHandle();
            ID3D12GraphicsCommandList* preloadCommandList = nullptr;//m_commandList.Get();
            const auto uploadedDataSize = UpdateSubresources(preloadCommandList, texture.Get(), textureUploadHeap.Get(), 0, 0, 1, &textureData);


            //&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
            //D3D12_HEAP_FLAG_NONE,
            //&CD3DX12_RESOURCE_DESC::Buffer(uploadBufferSize),
            //D3D12_RESOURCE_STATE_GENERIC_READ,
            //nullptr,
            //IID_PPV_ARGS(&textureUploadHeap)));

            int z = 0;
            z;

#if 0
    ThrowIfFailed(m_device->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
        D3D12_HEAP_FLAG_NONE,
        &textureDesc,
        D3D12_RESOURCE_STATE_COPY_DEST,
        nullptr,
        IID_PPV_ARGS(&m_texture)));


    D3D12_RESOURCE_DESC resourceDesc = {};
    resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    resourceDesc.Alignment = 0;
    resourceDesc.Width = textureHeader.mSizeX;
    resourceDesc.Height = textureHeader.mSizeY;
    resourceDesc.DepthOrArraySize = 1;
    resourceDesc.MipLevels = 1;
    resourceDesc.Format = textureFormat;
    resourceDesc.SampleDesc.Count = 1;
    resourceDesc.SampleDesc.Quality = 0;
    resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    resourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

    D3D12MA::ALLOCATION_DESC allocationDesc = {};
    allocationDesc.HeapType = D3D12_HEAP_TYPE_UPLOAD;

    D3D12MA::Allocation* allocation;
    ComPtr<ID3D12Resource> resource;

    HRESULT hr = allocator->CreateResource(
        &allocationDesc,
        &resourceDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        &allocation,
        IID_PPV_ARGS(&resource));

    error_handlers::ThrowOnError(hr, std::format("Could not allocate texture resource for tag: {}", conv::ToString(tag)));
    platform::SetDebugName(resource.Get(), allocation, "Texture", conv::ToString(tag));

    D3D12_RANGE emptyRange = { 0, 0 };
    void* mappedPtr;
    hr = resource->Map(0, &emptyRange, &mappedPtr);

    memcpy(mappedPtr, io::cast_data<const uint8_t>(texturePixels), io::size_data(texturePixels));

    resource->Unmap(0, nullptr);

    mResources[tag] = { resource, unique_obj<D3D12MA::Allocation>(allocation) };
    results.push_back(resource);
#endif
        }
    }

#endif

    return results;
}


//-------------------------------------------------------------------------------------------------
yaget::render::ComPtr<ID3D12Resource> yaget::render::TextureResources::GetResource(const io::Tag& tag)
{
    auto resources = GetResources(io::Tags{ tag });
    return !resources.empty() ? *resources.begin() : ComPtr<ID3D12Resource>{};
}
