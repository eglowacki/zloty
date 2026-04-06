#include "Core/ErrorHandlers.h"
#include "HashUtilities.h"
#include "Render/Commands/RenderCommandList.h"
#include "Render/Commands/RenderTarget.h"
#include "Render/Helpers/ResourceDescriptions.h"
#include "Render/Pipeline/RenderShaders.h"
#include "Render/Platform/SwapChain.h"
#include "Render/Commands/RenderCommandListStates.h"


//-------------------------------------------------------------------------------------------------
yaget::render::commands::RenderTarget::RenderTarget(ID3D12Device* device, int sizeX, int sizeY, DXGI_FORMAT renderTargetFormat, DXGI_FORMAT /*depthStencilFormat*/)
    : mSRVDescriptorHeap{ CreateDescriptorHeap(device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 1) }
    , mRTVDescriptorHeap{ CreateDescriptorHeap(device, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, 1) }
    , mDSVDescriptorHeap{}
    , mRenderTargetResource{}
    , mRenderTargetFormat{ renderTargetFormat }
    , mState{ D3D12_RESOURCE_STATE_RENDER_TARGET }
    , mClearColor{ 1.0f, 1.0f, 1.0f, 1.0f }
{
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

    //D3D12_CLEAR_VALUE optimizedClearValue = {};
    //optimizedClearValue.Format = renderTargetFormat;
    //memcpy(optimizedClearValue.Color, mClearColor, sizeof(optimizedClearValue.Color));

    HRESULT hr = device->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_ALLOW_ALL_BUFFERS_AND_TEXTURES, &renderTargetDesc, mState, nullptr/*&optimizedClearValue*/, IID_PPV_ARGS(&mRenderTargetResource));
    error_handlers::ThrowOnError(hr, std::format("Could not create render target for texture format: '{}' with sizeX: '{}', sizeY: '{}'.", magic_enum::enum_name(renderTargetFormat), sizeX, sizeY).c_str());

    D3D12_CPU_DESCRIPTOR_HANDLE rtvCpuDescriptorHandle{ mRTVDescriptorHeap->GetCPUDescriptorHandleForHeapStart() };
    D3D12_CPU_DESCRIPTOR_HANDLE srvCpuDescriptorHandle{ mSRVDescriptorHeap->GetCPUDescriptorHandleForHeapStart() };

    device->CreateRenderTargetView(mRenderTargetResource.Get(), nullptr, rtvCpuDescriptorHandle);
    device->CreateShaderResourceView(mRenderTargetResource.Get(), nullptr, srvCpuDescriptorHandle);
}


//-------------------------------------------------------------------------------------------------
yaget::render::commands::RenderTarget::RenderTarget(platform::SwapChain& swapChain)
    : mSRVDescriptorHeap{}
    , mRTVDescriptorHeap{ swapChain.GetRTVDescriptorHeap() }
    , mDSVDescriptorHeap{ swapChain.GetDSVDescriptorHeap() }
    , mRenderTargetResource{ nullptr }
    , mRenderTargetFormat{ swapChain.GetDescription().Format }
    , mState{ D3D12_RESOURCE_STATE_PRESENT }
    , mClearColor{ 1.0f, 1.0f, 1.0f, 1.0f }
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

    commands::ClearRenderTarget(commandList, colors::CornflowerBlue, mRenderTargetResource.Get(), mRTVDescriptorHeap.Get(), frameIndex);
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
void yaget::render::commands::RenderTarget::Present(const time::GameClock& gameClock, metrics::Channel& channel)
{
    if (mSwapChain)
    {
        mSwapChain->Present(gameClock, channel);
    }
}


//-------------------------------------------------------------------------------------------------
yaget::render::commands::RenderTargetStorage::RenderTargetStorage(ID3D12Device* device)
    : mDevice{ device }
{
}


//-------------------------------------------------------------------------------------------------
yaget::render::commands::RenderTargetStorage::~RenderTargetStorage() = default;


//-------------------------------------------------------------------------------------------------
yaget::render::commands::RenderTarget* yaget::render::commands::RenderTargetStorage::GetRenderTarget(const io::Tag& tag, uint32_t sizeX, uint32_t sizeY, DXGI_FORMAT renderTargetFormat, DXGI_FORMAT depthStencilFormat)
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
        mt::ReadLock readLock(mMutex);
        if (auto renderTarget = FindRenderTarget(tag, hashValue, mRenderTargetMap))
        {
            return renderTarget;
        }
    }

    mt::WriteLock readLock(mMutex);
    if (auto renderTarget = FindRenderTarget(tag, hashValue, mRenderTargetMap))
    {
        return renderTarget;
    }

    auto renderTarget = std::make_shared<RenderTarget>(mDevice, sizeX, sizeY, renderTargetFormat, depthStencilFormat);
    mRenderTargetMap[tag] = { .mHash = hashValue, .mRenderTarget = renderTarget };

    return renderTarget.get();
}


//-------------------------------------------------------------------------------------------------
yaget::render::commands::RenderTarget* yaget::render::commands::RenderTargetStorage::AliasRenderTarget(const io::Tag& tag, platform::SwapChain& swapChain)
{
    uint64_t hashValue = tag.Hash();

    {
        mt::ReadLock readLock(mMutex);
        if (auto renderTarget = FindRenderTarget(tag, hashValue, mRenderTargetMap))
        {
            return renderTarget;
        }
    }

    mt::WriteLock readLock(mMutex);
    if (auto renderTarget = FindRenderTarget(tag, hashValue, mRenderTargetMap))
    {
        return renderTarget;
    }

    auto renderTarget = std::make_shared<RenderTarget>(swapChain);
    mRenderTargetMap[tag] = { .mHash = hashValue, .mRenderTarget = renderTarget };

    return renderTarget.get();
}


//-------------------------------------------------------------------------------------------------
void yaget::render::commands::RenderTargetStorage::ResetAll(const app::WindowFrame& /*windowFrame*/)
{
    mt::WriteLock readLock(mMutex);
    mRenderTargetMap.clear();
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

    return renderTargetStorage.GetRenderTarget(tag, chainDesc.Width, chainDesc.Height, chainDesc.Format, depthStencilDesc.Format);
}
