#include "Core/ErrorHandlers.h"
#include "Json/JsonHelpers.h"
#include "Render/Device.h"
#include "Render/Pipeline/RenderShaders.h"
#include "Render/Pipeline/RenderTextures.h"
#include "Render/Platform/Adapter.h"
#include "Render/Platform/D3D12MemAlloc.h"
#include "Streams/Guid.h"
#include "VTS/ResolvedAssets.h"

#include <d3dx12.h>

namespace
{
    DXGI_FORMAT GetTextureFormat(int numComponents)
    {
        DXGI_FORMAT textureFormat = DXGI_FORMAT_UNKNOWN;
        switch (numComponents)
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
                YLOG_ERROR("REND", "There is no support for '%d' components image.", numComponents);
        }

        return textureFormat;
    }
}


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
void yaget::render::RenderTextures::Preload(const io::Tags& tags)
{
    GetTextures(tags);
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
yaget::render::TextureResources::~TextureResources()// = default;
{
    int z = 0;
    z;
}

yaget::render::ComPtr<ID3D12DescriptorHeap> yaget::render::TextureResources::GetResourceView(const io::Tag& tag)
{
    mt::ReadLock readLocker(mSharedMutex);
    if (auto it = mResources.find(tag); it != mResources.end())
    {
        return it->second.mDescriptorHeap;
    }

    YLOG_ERROR("REND", "Could not find texture resource view data for tag: '%s'.", yaget::conv::ToString(tag).c_str());
    return {};
}


//-------------------------------------------------------------------------------------------------
std::vector<yaget::render::ComPtr<ID3D12Resource>> yaget::render::TextureResources::GetResources(const io::Tags& tags)
{
    std::vector<ComPtr<ID3D12Resource>> results;

    for (const auto& tag : tags)
    {
        {
            mt::ReadLock readLocker(mSharedMutex);
            if (auto it = mResources.find(tag); it != mResources.end())
            {
                results.push_back(it->second.mResource);
                continue;
            }
        }

        auto textureBuffer = mRenderTextures.GetTexture(tag);
        if (!io::size_data(textureBuffer))
        {
            // NOTE(eg) Should we return some kind of built-in texture placeholder, 
            // in similar manner as we do it in RenderShaders (missing vertex or pixel shader)
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

            //--------------------------------------------------
            // Describe and create a Texture2D.
            DXGI_FORMAT textureFormat = GetTextureFormat(textureHeader.mComponents);

            D3D12MA::ALLOCATION_DESC heapDesc = {};
            heapDesc.HeapType = D3D12_HEAP_TYPE_DEFAULT;
            heapDesc.ExtraHeapFlags = D3D12_HEAP_FLAG_NONE;

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

            unique_obj<D3D12MA::Allocation> textureAllocation(allocation);
            platform::SetDebugName(texture.Get(), textureAllocation.get(), "Texture2D", conv::ToString(tag));

            const auto uploadBufferSize = GetRequiredIntermediateSize(texture.Get(), 0, 1);

            //--------------------------------------------------
            // Create the GPU upload buffer.
            D3D12MA::ALLOCATION_DESC uploadHeapDesc = {};
            uploadHeapDesc.HeapType = D3D12_HEAP_TYPE_UPLOAD;
            uploadHeapDesc.ExtraHeapFlags = D3D12_HEAP_FLAG_NONE;

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

            unique_obj<D3D12MA::Allocation> textureUploadAllocation(uploadAllocation);
            platform::SetDebugName(textureUploadHeap.Get(), textureUploadAllocation.get(), "UploadTexture", conv::ToString(tag));

            //--------------------------------------------------
            // Copy data to the intermediate upload heap and then schedule a copy 
            // from the upload heap to the Texture2D.
            D3D12_SUBRESOURCE_DATA textureData = {};
            textureData.pData = io::cast_data<uint8_t>(texturePixels);
            textureData.RowPitch = textureHeader.GetStride();
            textureData.SlicePitch = textureHeader.GetImageSize();

            auto framerHandler = mDevice.GetCopyCommands();
            auto commandList = framerHandler.BeginFrame(nullptr);

            auto preloadCommandList = commandList->GetDeviceCommandList();

            auto subresourceResult = UpdateSubresources(preloadCommandList, texture.Get(), textureUploadHeap.Get(), 0, 0, 1, &textureData);
            auto resultEven = subresourceResult == uploadBufferSize; resultEven;

            auto resourceBarrier = CD3DX12_RESOURCE_BARRIER::Transition(texture.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
            preloadCommandList->ResourceBarrier(1, &resourceBarrier);

            //--------------------------------------------------
            // Describe and create a SRV for the texture.
            D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
            srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
            srvDesc.Format = textureDesc.Format;
            srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
            srvDesc.Texture2D.MipLevels = 1;

            auto d3dDevice = mDevice.GetAdapter().GetDevice();
            auto srvHeap = CreateDescriptorHeap(d3dDevice, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 1);

            d3dDevice->CreateShaderResourceView(texture.Get(), &srvDesc, srvHeap->GetCPUDescriptorHandleForHeapStart());

            framerHandler.EndFrame();

            mResources.insert({ tag, ResourceData{ std::move(textureAllocation), texture, srvHeap } });
            results.push_back(texture);
        }
    }

    return results;
}


//-------------------------------------------------------------------------------------------------
void yaget::render::TextureResources::Preload(const io::Tags& tags)
{
    GetResources(tags);
}


//-------------------------------------------------------------------------------------------------
yaget::render::ComPtr<ID3D12Resource> yaget::render::TextureResources::GetResource(const io::Tag& tag)
{
    auto resources = GetResources(io::Tags{ tag });
    return !resources.empty() ? *resources.begin() : ComPtr<ID3D12Resource>{};
}
