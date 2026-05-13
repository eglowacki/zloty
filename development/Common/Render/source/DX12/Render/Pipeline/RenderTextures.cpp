#include "Core/ErrorHandlers.h"
#include "Json/JsonHelpers.h"
#include "Render/Commands/RenderTarget.h"
#include "Render/Device.h"
#include "Render/Helpers/ResourceDescriptions.h"
#include "Render/Pipeline/RenderShaders.h"
#include "Render/Pipeline/RenderTextures.h"
#include "Render/Platform/Adapter.h"
#include "Render/Platform/D3D12MemAlloc.h"
#include "Render/PlaceholderAssets/PlaceholderAssets.h"
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

    auto result = GetTextures(io::Tags{ tag }, nullptr);
    return !result.empty() ? *result.begin() : io::Buffer{};
}


//-------------------------------------------------------------------------------------------------
std::vector<yaget::io::Buffer> yaget::render::RenderTextures::GetTextures(const io::Tags& tags, comp::gs::mt::InitCounter* counter)
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

        if (counter)
        {
            ++(*counter);
        }

        results.push_back(result);
    }

    return results;
}


//-------------------------------------------------------------------------------------------------
void yaget::render::RenderTextures::Preload(const io::Tags& tags, comp::gs::mt::InitCounter& counter)
{
    GetTextures(tags, &counter);
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
ID3D12DescriptorHeap* yaget::render::TextureResources::GetResourceView(const io::Tag& tag) const
{
    auto resources = GetResourceViews(io::Tags{ tag });

    YLOG_CERROR("REND", !resources.empty(), "Could not find texture resource view data for tag: '%s'.", yaget::conv::ToString(tag).c_str());
    return !resources.empty() ? *resources.begin() : nullptr;
}


//-------------------------------------------------------------------------------------------------
std::vector<ID3D12DescriptorHeap*> yaget::render::TextureResources::GetResourceViews(const io::Tags& tags) const
{
    std::vector<ID3D12DescriptorHeap*> results;

    mt::ReadLock readLocker(mSharedMutex);
    for (const auto& tag : tags)
    {
        if (auto it = mResources.find(tag); it != mResources.end())
        {
            results.push_back(it->second.mDescriptorHeap.Get());
        }
    }

    return results;
}


//-------------------------------------------------------------------------------------------------
std::vector<ID3D12Resource*> yaget::render::TextureResources::GetResources(const io::Tags& tags, comp::gs::mt::InitCounter* counter)
{
    std::vector<ID3D12Resource*> results;
    io::Tags tagsToLoad;
    {
        mt::ReadLock readLocker(mSharedMutex);
        for (const auto& tag : tags)
        {
            if (auto it = mResources.find(tag); it != mResources.end())
            {
                results.push_back(it->second.mResource.Get());

                if (counter)
                {
                    ++(*counter);
                }

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
    auto commandList = framerHandler.BeginFrame(nullptr, nullptr);
    auto preloadCommandList = commandList->GetDeviceCommandList();

    mt::WriteLock writeLocker(mSharedMutex);
    for (const auto& tag : tagsToLoad)
    {
        // it's possible that between the time we released read lock and acquired write lock, 
        // another thread loaded the same texture, so we need to check again if resource is already in map
        if (auto it = mResources.find(tag); it != mResources.end())
        {
            results.push_back(it->second.mResource.Get());
            if (counter)
            {
                ++(*counter);
            }
            continue;
        }

        auto textureBuffer = mRenderTextures.GetTexture(tag);

        auto [textureHeader, texturePixels] = io::TextureAsset::ParseBuffer(textureBuffer);
        if (!textureHeader.GetImageSize())
        {
            YLOG_ERROR("REND", "Could not find texture data for tag: '%s', replacing with built-in placeholder.", yaget::conv::ToString(tag).c_str());
            auto placeholderTexture = placeholders::GetTextureData();
            textureHeader = static_cast<image::Header>(placeholderTexture);
            texturePixels = placeholderTexture.mPixels;

            error_handlers::ThrowOnError(io::size_data(texturePixels) > 0, "Could not get built-in texture");
        }

        if (textureHeader.mComponents == 3)
        {
            YLOG_ERROR("REND", "There is no support for 24 bit images. Tag: '%s'", yaget::conv::ToString(tag).c_str());
            continue;
        }

        auto allocator = mDevice.GetAdapter().GetAllocator();

        DXGI_FORMAT textureFormat = GetTextureFormat(textureHeader.mComponents);
        helpers::SourceGpuParameters sourceGpuParameters = helpers::MakeTextureBufferParameters(io::cast_data<const uint8_t>(texturePixels), textureHeader.mSizeX, textureHeader.mSizeY, textureFormat, textureHeader.GetStride(), textureHeader.GetImageSize());

        helpers::GpuResourceResult gpuResourceResult = helpers::CreateGpuResource(tag, sourceGpuParameters, preloadCommandList, allocator);

        //--------------------------------------------------
        // Describe and create a SRV for the texture.
        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srvDesc.Format = textureFormat;
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Texture2D.MipLevels = 1;

        auto d3dDevice = mDevice.GetAdapter().GetDevice();
        auto srvHeap = helpers::CreateDescriptorHeap(d3dDevice, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 1);

        ID3D12Resource* texture = gpuResourceResult.mGpuAllocation->GetResource();
        auto uploadAllocation = gpuResourceResult.mUploadAllocation;
        auto allocation = gpuResourceResult.mGpuAllocation;

        d3dDevice->CreateShaderResourceView(texture, &srvDesc, srvHeap->GetCPUDescriptorHandleForHeapStart());

        allocationsToKeepAlive.push_back(unique_obj<D3D12MA::Allocation>{ uploadAllocation });

        mResources.insert({ tag, ResourceData{ unique_obj<D3D12MA::Allocation>{ allocation }, texture, srvHeap } });
        results.push_back(texture);
        if (counter)
        {
            ++(*counter);
        }
    }

    framerHandler.EndFrame();

    return results;
}


//-------------------------------------------------------------------------------------------------
void yaget::render::TextureResources::AttachRenderTarget(const io::Tag& tag, const commands::RenderTarget* renderTarget)
{
    mt::WriteLock writeLocker(mSharedMutex);
    mResources[tag] = ResourceData{ unique_obj<D3D12MA::Allocation>{}, renderTarget->Resource(), renderTarget->SRVDescriptorHeap() };
}


//-------------------------------------------------------------------------------------------------
void yaget::render::TextureResources::Preload(const io::Tags& tags, comp::gs::mt::InitCounter& counter)
{
    GetResources(tags, &counter);
}


//-------------------------------------------------------------------------------------------------
ID3D12Resource* yaget::render::TextureResources::GetResource(const io::Tag& tag)
{
    auto resources = GetResources(io::Tags{ tag }, nullptr);
    return !resources.empty() ? *resources.begin() : nullptr;
}
