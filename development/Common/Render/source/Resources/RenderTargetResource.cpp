#include "Resources/RenderTargetResource.h"
#include "VTS/RenderResolvedAssets.h"
#include "Resources/ResourceView.h"
#include "Device.h"
#include "RenderHelpers.h"


yaget::render::RenderTargetResource::RenderTargetResource(Device& device, std::shared_ptr<io::render::RenderTargetAsset> asset)
    : ResourceView(device, asset->mTag, std::type_index(typeid(RenderTargetResource)))
    , mDimension(64, 64)
    , mClearColor(0.0f, 0.2f, 0.4f, 1.0f)
    , mName(asset->mTag.mName)
{
    std::size_t hashValue = asset->mTag.Hash();
    SetHashValue(hashValue);

    Device::ID3D11Device_t* d3dDevice = mDevice.GetDevice();

    const uint32_t texSize = 64;

    D3D11_TEXTURE2D_DESC textureDesc = {};
    // Setup the texture description.
    // We will have our map be a square
    // We will need to have this texture bound as a render target AND a shader resource
    textureDesc.Width = mDimension.first;
    textureDesc.Height = mDimension.second;
    textureDesc.MipLevels = 1;
    textureDesc.ArraySize = 1;
    //textureDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
    // NOTE: need a way to data drive these options/flags
    textureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    textureDesc.SampleDesc.Count = 1;
    // NOTE: need a way to data drive these options/flags
    textureDesc.Usage = D3D11_USAGE_DEFAULT;
    textureDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
    textureDesc.CPUAccessFlags = 0;
    textureDesc.MiscFlags = 0;

    // Create the texture

    HRESULT hr = d3dDevice->CreateTexture2D(&textureDesc, nullptr, &mTextureMap);
    YAGET_UTIL_THROW_ON_RROR(hr, "CreateTexture2D for render target failed");

    /////////////////////// Map's Render Target
    // Setup the description of the render target view.
    D3D11_RENDER_TARGET_VIEW_DESC renderTargetViewDesc = {};
    renderTargetViewDesc.Format = textureDesc.Format;
    renderTargetViewDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
    renderTargetViewDesc.Texture2D.MipSlice = 0;

    // Create the render target view.
    hr = d3dDevice->CreateRenderTargetView(mTextureMap.Get(), &renderTargetViewDesc, &mViewMap);
    YAGET_UTIL_THROW_ON_RROR(hr, "CreateRenderTargetView for render target failed");
    YAGET_SET_DEBUG_NAME(mViewMap.Get(), mName);

    /////////////////////// Map's Shader Resource View
    // Setup the description of the shader resource view.
    D3D11_SHADER_RESOURCE_VIEW_DESC shaderResourceViewDesc = {};
    shaderResourceViewDesc.Format = textureDesc.Format;
    shaderResourceViewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    shaderResourceViewDesc.Texture2D.MostDetailedMip = 0;
    shaderResourceViewDesc.Texture2D.MipLevels = 1;

    //// Create the shader resource view, used only if this target will be using as an input to shader (AsSampler)
    //hr = d3dDevice->CreateShaderResourceView(mTextureMap.Get(), &shaderResourceViewDesc, &mShaderViewMap);
    //YAGET_UTIL_THROW_ON_RROR(hr, "CreateShaderResourceView for render target failed");
    //YAGET_SET_DEBUG_NAME(mShaderViewMap.Get(), mName);

    //// Describe the Sample State
    //D3D11_SAMPLER_DESC sampDesc = {};
    //sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    //sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
    //sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
    //sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
    //sampDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
    //sampDesc.MinLOD = 0;
    //sampDesc.MaxLOD = D3D11_FLOAT32_MAX;

    ////Create the Sample State, used only if this target will be using as an input to shader (AsSampler)
    //hr = d3dDevice->CreateSamplerState(&sampDesc, &mSamplerSate);
    //YAGET_UTIL_THROW_ON_RROR(hr, "CreateSamplerState for render target failed");
    //YAGET_SET_DEBUG_NAME(mSamplerSate.Get(), mName);
    //yaget::render::TextureMetaResource::TextureMetaResource(Device & device, std::shared_ptr<io::render::ImageMetaAsset> asset)

    // ---------------------------------------------------------------------------------------
    D3D11_TEXTURE2D_DESC depthStencilBufferDesc = {};

    depthStencilBufferDesc.ArraySize = 1;
    depthStencilBufferDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
    depthStencilBufferDesc.CPUAccessFlags = 0; // No CPU access required.
    depthStencilBufferDesc.Format = DXGI_FORMAT_D32_FLOAT_S8X24_UINT;// DXGI_FORMAT_D24_UNORM_S8_UINT;
    depthStencilBufferDesc.Width = mDimension.first;
    depthStencilBufferDesc.Height = mDimension.second;
    depthStencilBufferDesc.MipLevels = 1;
    depthStencilBufferDesc.SampleDesc.Count = 1;
    depthStencilBufferDesc.SampleDesc.Quality = 0;
    depthStencilBufferDesc.Usage = D3D11_USAGE_DEFAULT;

    hr = d3dDevice->CreateTexture2D(&depthStencilBufferDesc, nullptr, &mDepthStencilBuffer);
    YAGET_UTIL_THROW_ON_RROR(hr, "Could not initialize Depth & Stencil Buffers");
    YAGET_SET_DEBUG_NAME(mDepthStencilBuffer.Get(), mName);

    hr = d3dDevice->CreateDepthStencilView(mDepthStencilBuffer.Get(), nullptr, &mDepthStencilView);
    YAGET_UTIL_THROW_ON_RROR(hr, "Could not create Depth & Stencil view");
    YAGET_SET_DEBUG_NAME(mDepthStencilView.Get(), mName);

    D3D11_DEPTH_STENCIL_DESC depthStencilStateDesc = {};

    depthStencilStateDesc.DepthEnable = TRUE;
    depthStencilStateDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
    depthStencilStateDesc.DepthFunc = D3D11_COMPARISON_LESS;
    depthStencilStateDesc.StencilEnable = FALSE;

    hr = d3dDevice->CreateDepthStencilState(&depthStencilStateDesc, &mDepthStencilState);
    YAGET_UTIL_THROW_ON_RROR(hr, "Could not create Depth & Stencil state");
    YAGET_SET_DEBUG_NAME(mDepthStencilState.Get(), mName);

    SetPlatformResource(mTextureMap.Get());
}


yaget::render::RenderTargetResource::~RenderTargetResource()
{
}


bool yaget::render::RenderTargetResource::Activate()
{
    Device::ID3D11DeviceContext_t* d3dDeviceContext = mDevice.GetDeviceContext();

    ID3D11RenderTargetView* viewMaps[] = { mViewMap.Get() };
    d3dDeviceContext->OMSetRenderTargets(1, viewMaps, mDepthStencilView.Get());
    d3dDeviceContext->OMSetDepthStencilState(mDepthStencilState.Get(), 1);

    d3dDeviceContext->ClearRenderTargetView(mViewMap.Get(), mClearColor);
    d3dDeviceContext->ClearDepthStencilView(mDepthStencilView.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

    return true;
}

