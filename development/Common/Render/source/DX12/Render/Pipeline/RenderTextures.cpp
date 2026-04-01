#include "Core/ErrorHandlers.h"
#include "Json/JsonHelpers.h"
#include "Render/Device.h"
#include "Render/Pipeline/RenderShaders.h"
#include "Render/Pipeline/RenderTextures.h"
#include "Render/Platform/Adapter.h"
#include "Render/Platform/D3D12MemAlloc.h"
#include "Streams/Guid.h"
#include "VTS/ResolvedAssets.h"
#include "Render/Helpers/ResourceDescriptions.h"

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
    io::Tags tagsToLoad;
    {
        mt::ReadLock readLocker(mSharedMutex);
        for (const auto& tag : tags)
        {
            if (auto it = mResources.find(tag); it != mResources.end())
            {
                results.push_back(it->second.mResource);
                continue;
            }

            tagsToLoad.push_back(tag);
        }

        if (tagsToLoad.empty())
        {
            return results;
        }
    }

    std::vector<unique_obj<D3D12MA::Allocation>> allocationsToKeepAlive;
    auto framerHandler = mDevice.GetCopyCommands();
    auto commandList = framerHandler.BeginFrame(nullptr);
    auto preloadCommandList = commandList->GetDeviceCommandList();

    mt::WriteLock writeLocker(mSharedMutex);
    for (const auto& tag : tagsToLoad)
    {
        // it's possible that between the time we released read lock and acquired write lock, 
        // another thread loaded the same texture, so we need to check again if resource is already in map
        if (auto it = mResources.find(tag); it != mResources.end())
        {
            results.push_back(it->second.mResource);
            continue;
        }

        auto textureBuffer = mRenderTextures.GetTexture(tag);
        if (!io::size_data(textureBuffer))
        {
            // NOTE(eg) Should we return some kind of built-in texture placeholder, 
            // in similar manner as we do it in RenderShaders (missing vertex or pixel shader)
            YLOG_ERROR("REND", "Could not find texture data for tag: '%s'.", yaget::conv::ToString(tag).c_str());
            continue;
        }

        auto [textureHeader, texturePixels] = io::TextureAsset::ParseBuffer(textureBuffer);
        if (textureHeader.GetImageSize())
        {
            if (textureHeader.mComponents == 3)
            {
                YLOG_ERROR("REND", "There is no support for 24 bit images. Tag: '%s'", yaget::conv::ToString(tag).c_str());
                continue;
            }

            auto allocator = mDevice.GetAdapter().GetAllocator();

            DXGI_FORMAT textureFormat = GetTextureFormat(textureHeader.mComponents);

            helpers::SourceGpuParameters sourceGpuParameters
            {
                .mSizeX = textureHeader.mSizeX,
                .mSizeY = textureHeader.mSizeY,
                .mFormat = textureFormat,
                .mData = io::cast_data<const uint8_t>(texturePixels),
                .mStride = textureHeader.GetStride(),
                .mSliceSize = textureHeader.GetImageSize(),
                .mDimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D,
                .mLayout = D3D12_TEXTURE_LAYOUT_UNKNOWN,
                .mResourceState = D3D12_RESOURCE_STATE_COMMON,
                .mTransitionTo = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
            };

            helpers::GpuResourceResult gpuResourceResult = helpers::CreateGpuResource(tag, sourceGpuParameters, preloadCommandList, allocator);

            //--------------------------------------------------
            // Describe and create a SRV for the texture.
            D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
            srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
            srvDesc.Format = textureFormat;
            srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
            srvDesc.Texture2D.MipLevels = 1;

            auto d3dDevice = mDevice.GetAdapter().GetDevice();
            auto srvHeap = CreateDescriptorHeap(d3dDevice, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 1);

            ID3D12Resource* texture = gpuResourceResult.mGpuAllocation->GetResource();
            auto uploadAllocation = gpuResourceResult.mUploadAllocation;
            auto allocation = gpuResourceResult.mGpuAllocation;

            d3dDevice->CreateShaderResourceView(texture, &srvDesc, srvHeap->GetCPUDescriptorHandleForHeapStart());

            allocationsToKeepAlive.push_back(unique_obj<D3D12MA::Allocation>{ uploadAllocation });

            mResources.insert({ tag, ResourceData{ unique_obj<D3D12MA::Allocation>{ allocation }, texture, srvHeap } });
            results.push_back(texture);
        }
    }

    framerHandler.EndFrame();

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
