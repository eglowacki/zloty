#include "Core/ErrorHandlers.h"
#include "HashUtilities.h"
#include "Json/JsonHelpers.h"
#include "Render/Commands/RenderCommandList.h"
#include "Render/Commands/RenderCommandListStates.h"
#include "Render/Commands/RenderTarget.h"
#include "Render/Helpers/ResourceDescriptions.h"
#include "Render/Pipeline/RenderShaders.h"
#include "Render/Pipeline/RenderTextures.h"
#include "Render/Platform/SwapChain.h"


namespace
{
    enum class TargetType
    {
        Texture,
        FromSwapChain,
        AliasSwapChain
    };

    struct TargetData
    {
        TargetType mType;
        uint32_t mSizeX{};
        uint32_t mSizeY{};
        DXGI_FORMAT mRenderFormat;
        DXGI_FORMAT mDepthStencilFormat;
    };


    //-------------------------------------------------------------------------------------------------
    void to_json(nlohmann::json& j, const TargetData& targetData)
    {
        j["Type"] = magic_enum::enum_name(targetData.mType);
        j["SizeX"] = targetData.mSizeX;
        j["SizeY"] = targetData.mSizeY;
        j["RenderFormat"] = magic_enum::enum_name(targetData.mRenderFormat);
        j["DepthStencilFormat"] = magic_enum::enum_name(targetData.mDepthStencilFormat);
    }


    //-------------------------------------------------------------------------------------------------
    void from_json(const nlohmann::json& j, TargetData& targetData)
    {
        targetData.mType = yaget::json::from_json_enum<TargetType>(j, "Type", "Texture");
        targetData.mSizeX = yaget::json::GetValue(j, "SizeX", targetData.mSizeX);
        targetData.mSizeY = yaget::json::GetValue(j, "SizeY", targetData.mSizeY);
        targetData.mRenderFormat = yaget::json::from_json_enum<DXGI_FORMAT>(j, "RenderFormat", "DXGI_FORMAT_UNKNOWN");
        targetData.mDepthStencilFormat = yaget::json::from_json_enum<DXGI_FORMAT>(j, "DepthStencilFormat", "DXGI_FORMAT_UNKNOWN");
    }
    

    //-------------------------------------------------------------------------------------------------
    yaget::render::ComPtr<ID3D12DescriptorHeap> SetupDepthStencilDescriptorHeap(ID3D12Device* device, DXGI_FORMAT depthStencilFormat, ID3D12DescriptorHeap* descriptorHeap)
    {
        if (descriptorHeap)
        {
            return descriptorHeap;
        }

        if (depthStencilFormat == DXGI_FORMAT_UNKNOWN)
        {
            return {};
        }

        return yaget::render::helpers::CreateDescriptorHeap(device, D3D12_DESCRIPTOR_HEAP_TYPE_DSV, 1);
    }


    //-------------------------------------------------------------------------------------------------
    yaget::render::ComPtr<ID3D12Resource> SetupDepthStencilResource(ID3D12Device* device, int sizeX, int sizeY, DXGI_FORMAT depthStencilFormat, ID3D12DescriptorHeap* descriptorHeap, ID3D12Resource* resource)
    {
        if (resource)
        {
            return resource;
        }

        return yaget::render::helpers::CreateDepthStencilBuffer(device, descriptorHeap, sizeX, sizeY, depthStencilFormat);
    }


    //-------------------------------------------------------------------------------------------------
    yaget::render::ComPtr<ID3D12Resource> SetupRenderResource(ID3D12Device* device, int sizeX, int sizeY, DXGI_FORMAT renderTargetFormat, D3D12_RESOURCE_STATES state, ID3D12DescriptorHeap* rtvHeap, ID3D12DescriptorHeap* srvHeap)
    {
        yaget::render::ComPtr<ID3D12Resource> renderTargetResource;
        D3D12_HEAP_PROPERTIES heapProperties{};
        heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;

        D3D12_RESOURCE_DESC renderTargetDesc = {};
        renderTargetDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        renderTargetDesc.Alignment = 0;
        renderTargetDesc.Width = sizeX;
        renderTargetDesc.Height = sizeY;
        renderTargetDesc.DepthOrArraySize = 1;
        renderTargetDesc.MipLevels = 1;
        renderTargetDesc.Format = renderTargetFormat;
        renderTargetDesc.SampleDesc.Count = 1;
        renderTargetDesc.SampleDesc.Quality = 0;
        renderTargetDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
        renderTargetDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

        D3D12_CLEAR_VALUE optimizedClearValue = {};
        optimizedClearValue.Format = renderTargetFormat;
        //optimizedClearValue.Color[0] = 0.0f;
        //optimizedClearValue.Color[1] = 1.0f;
        //optimizedClearValue.Color[2] = 0.0f;
        //optimizedClearValue.Color[3] = 1.0f;
        memcpy(optimizedClearValue.Color, colors::Aqua, sizeof(optimizedClearValue.Color));

        HRESULT hr = device->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_ALLOW_ALL_BUFFERS_AND_TEXTURES, &renderTargetDesc, state, &optimizedClearValue, IID_PPV_ARGS(&renderTargetResource));
        yaget::error_handlers::ThrowOnError(hr, std::format("Could not create render target for texture format: '{}' with sizeX: '{}', sizeY: '{}'.", magic_enum::enum_name(renderTargetFormat), sizeX, sizeY).c_str());

        D3D12_CPU_DESCRIPTOR_HANDLE rtvCpuDescriptorHandle{ rtvHeap->GetCPUDescriptorHandleForHeapStart() };
        D3D12_CPU_DESCRIPTOR_HANDLE srvCpuDescriptorHandle{ srvHeap->GetCPUDescriptorHandleForHeapStart() };

        device->CreateRenderTargetView(renderTargetResource.Get(), nullptr, rtvCpuDescriptorHandle);
        device->CreateShaderResourceView(renderTargetResource.Get(), nullptr, srvCpuDescriptorHandle);

        return renderTargetResource;
        
    }


}
    

//-------------------------------------------------------------------------------------------------
yaget::render::commands::RenderTarget::RenderTarget(ID3D12Device* device, int sizeX, int sizeY, DXGI_FORMAT renderTargetFormat, DXGI_FORMAT depthStencilFormat, ID3D12DescriptorHeap* depthStencilDescriptorHeap, ID3D12Resource* depthStencilResource)
    : mSRVDescriptorHeap{ helpers::CreateDescriptorHeap(device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 1) }
    , mRTVDescriptorHeap{ helpers::CreateDescriptorHeap(device, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, 1) }
    , mDSVDescriptorHeap{ SetupDepthStencilDescriptorHeap(device, depthStencilFormat, depthStencilDescriptorHeap) }
    , mRenderTargetFormat{ renderTargetFormat }
    , mDepthStencilResource{ SetupDepthStencilResource(device, sizeX, sizeY, depthStencilFormat, mDSVDescriptorHeap.Get(), depthStencilResource) }
    , mState{ D3D12_RESOURCE_STATE_COMMON }
    , mClearColor{ colors::Aqua }
    , mRenderTargetResource{ SetupRenderResource(device, sizeX, sizeY, mRenderTargetFormat, mState, mRTVDescriptorHeap.Get(), mSRVDescriptorHeap.Get()) }
    , mSwapChain{ nullptr }
{
}


//-------------------------------------------------------------------------------------------------
yaget::render::commands::RenderTarget::RenderTarget(platform::SwapChain& swapChain)
    : mSRVDescriptorHeap{}
    , mRTVDescriptorHeap{ swapChain.GetRTVDescriptorHeap() }
    , mDSVDescriptorHeap{ swapChain.GetDSVDescriptorHeap() }
    , mRenderTargetResource{ nullptr }
    , mRenderTargetFormat{ swapChain.GetDescription().Format }
    , mState{ D3D12_RESOURCE_STATE_PRESENT }
    , mClearColor{ colors::Aqua }
    , mSwapChain{ &swapChain }
{
}


//-------------------------------------------------------------------------------------------------
yaget::render::commands::RenderTarget::~RenderTarget() = default;


//-------------------------------------------------------------------------------------------------
void yaget::render::commands::RenderTarget::BeginFrame(const CommandList* commandList)
{
    uint32_t frameIndex = mSwapChain ? mSwapChain->GetCurrentBackBufferIndex() : 0;
    if (mSwapChain)
    {
        mRenderTargetResource = mSwapChain->GetCurrentRenderTarget();
    }

    mState = commands::TransitionToRenderTarget(commandList, mState, mRenderTargetResource.Get(), mRTVDescriptorHeap.Get(), mDSVDescriptorHeap.Get(), frameIndex);

    commands::ClearRenderTarget(commandList, mClearColor, mRenderTargetResource.Get(), mRTVDescriptorHeap.Get(), frameIndex);
    if (mDSVDescriptorHeap)
    {
        commands::ClearDepthStencil(commandList, 1.0f, 0, mDSVDescriptorHeap.Get());
    }
}


//-------------------------------------------------------------------------------------------------
void yaget::render::commands::RenderTarget::EndFrame(const CommandList* commandList)
{
    if (mSwapChain)
    {
        mState = commands::TransitionFromTo(commandList, mRenderTargetResource.Get(), mState, D3D12_RESOURCE_STATE_PRESENT);
    }
    else
    {
        mState = commands::TransitionFromTo(commandList, mRenderTargetResource.Get(), mState, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    }
}


//-------------------------------------------------------------------------------------------------
bool yaget::render::commands::RenderTarget::Present(const time::GameClock& gameClock, metrics::Channel& channel)
{
    if (mSwapChain)
    {
        mSwapChain->Present(gameClock, channel);
        return true;
    }

    return false;
}


//-------------------------------------------------------------------------------------------------
yaget::render::commands::RenderTargetStorage::RenderTargetStorage(ID3D12Device* device, platform::SwapChain& swapChain, TextureResources& textureResources, io::VirtualTransportSystem& vts)
    : mDevice{ device }
    , mSwapChain{ swapChain }
    , mTextureResources{ textureResources }
    , mVTS{ vts }
{
}


//-------------------------------------------------------------------------------------------------
yaget::render::commands::RenderTargetStorage::~RenderTargetStorage() = default;


//-------------------------------------------------------------------------------------------------
yaget::render::commands::RenderTarget* yaget::render::commands::RenderTargetStorage::GetRenderTarget(const io::Tag& tag, uint32_t sizeX, uint32_t sizeY, DXGI_FORMAT renderTargetFormat, DXGI_FORMAT depthStencilFormat, ID3D12DescriptorHeap* depthStencilDescriptorHeap, ID3D12Resource* depthStencilResource)
{
    D3D12_FEATURE_DATA_FORMAT_SUPPORT formatSupport = { renderTargetFormat, D3D12_FORMAT_SUPPORT1_NONE, D3D12_FORMAT_SUPPORT2_NONE };
    HRESULT hr = mDevice->CheckFeatureSupport(D3D12_FEATURE_FORMAT_SUPPORT, &formatSupport, sizeof(formatSupport));
    error_handlers::ThrowOnError(hr, std::format("Could not CheckFeatureSupport for texture format: '{}'.", magic_enum::enum_name(renderTargetFormat)).c_str());

    UINT required = D3D12_FORMAT_SUPPORT1_TEXTURE2D | D3D12_FORMAT_SUPPORT1_RENDER_TARGET;
    auto result = (formatSupport.Support1 & required) == required;
    if (!result)
    {
        YLOG_ERROR("REND", std::format("Render Target format: '{}' does not support required features.", magic_enum::enum_name(renderTargetFormat)).c_str());
        return {};
    }

    uint64_t hashValue = 0;
    conv::hash_combine(hashValue, sizeX, sizeY, renderTargetFormat);

    {
        mt::ReadLock locker(mMutex);
        if (auto renderTarget = FindRenderTarget(tag, hashValue, mRenderTargetMap))
        {
            return renderTarget;
        }
    }

    mt::WriteLock locker(mMutex);
    if (auto renderTarget = FindRenderTarget(tag, hashValue, mRenderTargetMap))
    {
        return renderTarget;
    }

    auto renderTarget = std::make_shared<RenderTarget>(mDevice, sizeX, sizeY, renderTargetFormat, depthStencilFormat, depthStencilDescriptorHeap, depthStencilResource);
    mRenderTargetMap[tag] = { .mHash = hashValue, .mRenderTarget = renderTarget };

    return renderTarget.get();
}


//-------------------------------------------------------------------------------------------------
yaget::render::commands::RenderTarget* yaget::render::commands::RenderTargetStorage::AliasRenderTarget(const io::Tag& tag, platform::SwapChain& swapChain)
{
    uint64_t hashValue = tag.Hash();

    {
        mt::ReadLock locker(mMutex);
        if (auto renderTarget = FindRenderTarget(tag, hashValue, mRenderTargetMap))
        {
            return renderTarget;
        }
    }

    mt::WriteLock locker(mMutex);
    if (auto renderTarget = FindRenderTarget(tag, hashValue, mRenderTargetMap))
    {
        return renderTarget;
    }

    auto renderTarget = std::make_shared<RenderTarget>(swapChain);
    mRenderTargetMap[tag] = { .mHash = hashValue, .mRenderTarget = renderTarget };

    return renderTarget.get();
}


//-------------------------------------------------------------------------------------------------
yaget::render::commands::RenderTarget* yaget::render::commands::RenderTargetStorage::FindRenderTarget(const io::Tag& tag) const
{
    mt::ReadLock locker(mMutex);

    if (auto it = mRenderTargetMap.find(tag); it != mRenderTargetMap.end())
    {
        return it->second.mRenderTarget.get();
    }

    return nullptr;
}


//-------------------------------------------------------------------------------------------------
void yaget::render::commands::RenderTargetStorage::Preload(const io::Tags& tags)
{
    for (const auto& tag: tags)
    {
        TargetData renderTargetData = io::LoadBlob<TargetData>(mVTS, tag);

        RenderTarget* renderTarget{};
        switch (renderTargetData.mType)
        {
            case TargetType::Texture:
            {
                renderTarget = GetRenderTarget(tag, renderTargetData.mSizeX, renderTargetData.mSizeY, renderTargetData.mRenderFormat, renderTargetData.mDepthStencilFormat, nullptr, nullptr);
            }
            break;
            case TargetType::FromSwapChain:
                renderTarget = CreateRenderTargetFrom(tag, mSwapChain, *this);
                break;
            case TargetType::AliasSwapChain:
                AliasRenderTarget(tag, mSwapChain);
                break;
            default:
                YLOG_ERROR("REND", std::format("Unsupported RenderTarget type: '{}' for tag: '{}'.", magic_enum::enum_name<>(renderTargetData.mType), conv::ToString(tag)).c_str());
                continue;
        }

        if (renderTarget)
        {
            mTextureResources.AttachRenderTarget(tag, renderTarget);
        }
    }
}


//-------------------------------------------------------------------------------------------------
void yaget::render::commands::RenderTargetStorage::ResetAll(const app::WindowFrame& /*windowFrame*/)
{
    mt::WriteLock locker(mMutex);
    mRenderTargetMap.clear();
}


//-------------------------------------------------------------------------------------------------
void yaget::render::commands::RenderTargetStorage::PopulateMappings(io::VirtualTransportSystem::Section /*fileName*/, io::VirtualTransportSystem& /*vts*/)
{
    //PopulateMap(fileName, vts, ShaderOptionsMappings);
}


//-------------------------------------------------------------------------------------------------
void yaget::render::commands::RenderTargetStorage::SaveMappings(io::VirtualTransportSystem::Section /*fileName*/, io::VirtualTransportSystem& /*vts*/)
{
    //SaveMap(fileName, vts, ShaderOptionsMappings);
}


//-------------------------------------------------------------------------------------------------
yaget::render::commands::RenderTarget* yaget::render::commands::RenderTargetStorage::FindRenderTarget(const io::Tag& tag, size_t hashValue, const RenderTargetMap& renderTargetMap)
{
    if (auto it = renderTargetMap.find(tag); it != renderTargetMap.end())
    {
        if (it->second.mHash == hashValue)
        {
            return it->second.mRenderTarget.get();
        }
    }

    return nullptr;
}


//-------------------------------------------------------------------------------------------------
yaget::render::commands::RenderTarget* yaget::render::commands::CreateRenderTargetFrom(const io::Tag& tag, const platform::SwapChain& swapChain, RenderTargetStorage& renderTargetStorage)
{
    // this can be used to extract actual buffer size (window size)
    auto chainDesc = swapChain.GetDescription();
    auto depthStencilDesc = swapChain.GetDepthStencilDescription();
    //ID3D12DescriptorHeap* depthStencilDescriptorHeap, ID3D12Resource* depthStencilResource
    auto depthStencilDescriptorHeap = swapChain.GetDSVDescriptorHeap();
    auto depthStencilResource = swapChain.GetCurrentDepthStencil();

    return renderTargetStorage.GetRenderTarget(tag, chainDesc.Width, chainDesc.Height, chainDesc.Format, depthStencilDesc.Format, depthStencilDescriptorHeap, depthStencilResource);
}
