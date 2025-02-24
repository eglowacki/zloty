#include "RenderTarget.h"
#include "Device.h"
#include "Debugging/Assert.h"
#include "Gui/Support.h"
#include "Logger/YLog.h"
#include <ScreenGrab.h>
#include <filesystem>
#include <wincodec.h>

#include "Core/ErrorHandlers.h"

namespace fs = std::filesystem;
using namespace Microsoft::WRL;


yaget::render::RenderTarget::RenderTarget(Device& device, uint32_t width, uint32_t height, const std::string& name)
    : mDevice(device)
    , mSwapChain(width == RenderTarget::kDefaultSize && height == RenderTarget::kDefaultSize)
    , mDimension(width, height)
    , mName(name)
{
    YAGET_ASSERT((width == RenderTarget::kDefaultSize && height == RenderTarget::kDefaultSize) || (width != RenderTarget::kDefaultSize && height != RenderTarget::kDefaultSize),
        "Invalid set of dimensions for render target: Width: '%d', Height: '%d'.", width, height);

    //mRasterizerState = static_cast<uint32_t>(RenderTarget::ERasterizerState::WIRE_MODE);
    CreateResources(width, height);
}

yaget::render::RenderTarget::~RenderTarget()
{
}

void yaget::render::RenderTarget::SetRasterizerState(uint32_t rasterizerState)
{
    mRasterizerState = rasterizerState;
}

float yaget::render::RenderTarget::GetAspectRatio() const
{
    auto dimensions = Size<float>();
    return dimensions.first / dimensions.second;
}

void yaget::render::RenderTarget::BeginRender()
{
    Device::ID3D11DeviceContext_t* d3dDeviceContext = mDevice.GetDeviceContext();

    ID3D11RenderTargetView* viewMaps[] = { mViewMap.Get() };
    d3dDeviceContext->OMSetRenderTargets(1, viewMaps, mDepthStencilView.Get());
    d3dDeviceContext->OMSetDepthStencilState(mDepthStencilState.Get(), 1);

    auto dimensions = Size<float>();
    D3D11_VIEWPORT viewport = {};
    viewport.TopLeftX = 0;
    viewport.TopLeftY = 0;
    viewport.Width = dimensions.first;
    viewport.Height = dimensions.second;
    viewport.MinDepth = 0;
    viewport.MaxDepth = 1;
    d3dDeviceContext->RSSetViewports(1, &viewport);

    d3dDeviceContext->ClearRenderTargetView(mViewMap.Get(), mClearColor);
    d3dDeviceContext->ClearDepthStencilView(mDepthStencilView.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

    bool bDeafultState = !(mRasterizerState & static_cast<uint32_t>(ERasterizerState::WIRE_MODE));
    d3dDeviceContext->RSSetState(bDeafultState ? mDefaultRasterizerState.Get() : mWireRasterizerState.Get());

    if (mSwapChain)
    {
        yaget::gui::NewFrame();
    }

    ProcessStage(ProcessorType::PreFrame);
}

void yaget::render::RenderTarget::EndRender()
{
    ProcessStage(ProcessorType::PostFrame);

    if (mSwapChain)
    {
        yaget::gui::Draw();
        mDevice.GetSwapChain()->Present(mVSync, 0);
    }
}

void yaget::render::RenderTarget::ProcessStage(ProcessorType processorType)
{
    auto& currentStage = mProcessorsStage[processorType];
    while (!currentStage.empty())
    {
        currentStage.front()();
        currentStage.pop();
    }
}

void yaget::render::RenderTarget::AddProcessor(ProcessorType processorType, ProcessorFunction processorFunction)
{
    YAGET_ASSERT(dev::CurrentThreadIds().IsThreadRender(), "Render Target AddProcessor must be called from RENDER thread.");

    mProcessorsStage[processorType].push(processorFunction);
}

void yaget::render::RenderTarget::UseAsSampler()
{
    YAGET_ASSERT(mSwapChain, "UseAsSampler is only valid for non swap-chain.");
    Device::ID3D11DeviceContext_t* d3dDeviceContext = mDevice.GetDeviceContext();

    d3dDeviceContext->PSSetShaderResources(0, 1, mShaderViewMap.GetAddressOf());
    d3dDeviceContext->PSSetSamplers(0, 1, mSamplerSate.GetAddressOf());
}

bool yaget::render::RenderTarget::IsScreenshotToTake() const
{
    return mTakeScreenshot;
}

void yaget::render::RenderTarget::TakeScreenshot(const char* fileName)
{
    mScreenshotFileName = fileName;
    mTakeScreenshot = true;
}

void yaget::render::RenderTarget::SaveToFile() const
{
    std::string imageFileName = mScreenshotFileName;
    if (imageFileName.empty())
    {
        fs::path textureFilePath(util::ExpendEnv("$(AssetFolder)", nullptr));

        static int index = 0;
        int mapSize = 1024;
        std::string nextFileName = fmt::format("ScreenShot_{}_{}.png", index++, mapSize);

        textureFilePath /= fs::path("Textures") / fs::path(nextFileName);
        imageFileName = textureFilePath.string();
    }

    Device::ID3D11DeviceContext_t* d3dDeviceContext = mDevice.GetDeviceContext();

    ComPtr<ID3D11Texture2D> backBuffer;
    if (mSwapChain)
    {
        const HRESULT hr = mDevice.GetSwapChain()->GetBuffer(0, __uuidof(ID3D11Texture2D), &backBuffer);
        error_handlers::ThrowOnError(hr, "Could not get Back Buffer from swap chain for screenshot");
    }
    else
    {
        backBuffer = mTextureMap;
    }

    fs::path imageFullPath(imageFileName);
    const GUID* imageFormat = nullptr;
    if (imageFullPath.extension() == fs::path(".png"))
    {
        imageFormat = &GUID_ContainerFormatPng;
    }
    else if (imageFullPath.extension() == fs::path(".jpg"))
    {
        imageFormat = &GUID_ContainerFormatJpeg;
    }

    YAGET_ASSERT(imageFormat, "imageFormat is not set, supported formats are 'png, jpg', file name: '%s'", imageFileName.c_str());

    std::wstring fileNameW = conv::utf8_to_wide(imageFullPath.string());
    const HRESULT hr = DirectX::SaveWICTextureToFile(d3dDeviceContext, backBuffer.Get(), *imageFormat, fileNameW.c_str());
    error_handlers::ThrowOnError(hr, "Could not save screenshot of render target to a file.");

    const_cast<render::RenderTarget*>(this)->mTakeScreenshot = false;

    YLOG_NOTICE("DEVV", "Saved screenshot at: '%s'.", imageFileName.c_str());
}

void yaget::render::RenderTarget::FreeResources()
{
    mTextureMap = nullptr;
    mViewMap = nullptr;
    mShaderViewMap = nullptr;
    mSamplerSate = nullptr;

    mDepthStencilBuffer = nullptr;
    mDepthStencilView = nullptr;
    mDepthStencilState = nullptr;
    mDimension = { 0, 0 };
}

void yaget::render::RenderTarget::CreateResources(uint32_t width, uint32_t height)
{
    FreeResources();

    mDimension = { width, height };
    mVSync = dev::CurrentConfiguration().mInit.VSync;

    Device::ID3D11Device_t* d3dDevice = mDevice.GetDevice();

    if (mSwapChain)
    {
        ComPtr<ID3D11Texture2D> backBuffer;
        HRESULT hr = mDevice.GetSwapChain()->GetBuffer(0, __uuidof(ID3D11Texture2D), &backBuffer);
        error_handlers::ThrowOnError(hr, "Could not initialize DX11 Back Buffer");

        hr = d3dDevice->CreateRenderTargetView(backBuffer.Get(), nullptr, &mViewMap);
        error_handlers::ThrowOnError(hr, "Could not create render target view");
        YAGET_SET_DEBUG_NAME(mViewMap.Get(), mName);

        D3D11_TEXTURE2D_DESC desc{};
        backBuffer->GetDesc(&desc);

        mDimension = { desc.Width, desc.Height };
    }
    else
    {
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
        error_handlers::ThrowOnError(hr, "CreateTexture2D for render target failed");

        /////////////////////// Map's Render Target
        // Setup the description of the render target view.
        D3D11_RENDER_TARGET_VIEW_DESC renderTargetViewDesc = {};
        renderTargetViewDesc.Format = textureDesc.Format;
        renderTargetViewDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
        renderTargetViewDesc.Texture2D.MipSlice = 0;

        // Create the render target view.
        hr = d3dDevice->CreateRenderTargetView(mTextureMap.Get(), &renderTargetViewDesc, &mViewMap);
        error_handlers::ThrowOnError(hr, "CreateRenderTargetView for render target failed");
        YAGET_SET_DEBUG_NAME(mViewMap.Get(), mName);

        /////////////////////// Map's Shader Resource View
        // Setup the description of the shader resource view.
        D3D11_SHADER_RESOURCE_VIEW_DESC shaderResourceViewDesc = {};
        shaderResourceViewDesc.Format = textureDesc.Format;
        shaderResourceViewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
        shaderResourceViewDesc.Texture2D.MostDetailedMip = 0;
        shaderResourceViewDesc.Texture2D.MipLevels = 1;

        // Create the shader resource view.
        hr = d3dDevice->CreateShaderResourceView(mTextureMap.Get(), &shaderResourceViewDesc, &mShaderViewMap);
        error_handlers::ThrowOnError(hr, "CreateShaderResourceView for render target failed");
        YAGET_SET_DEBUG_NAME(mShaderViewMap.Get(), mName);

        // Describe the Sample State
        D3D11_SAMPLER_DESC sampDesc = {};
        sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
        sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
        sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
        sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
        sampDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
        sampDesc.MinLOD = 0;
        sampDesc.MaxLOD = D3D11_FLOAT32_MAX;

        //Create the Sample State
        hr = d3dDevice->CreateSamplerState(&sampDesc, &mSamplerSate);
        error_handlers::ThrowOnError(hr, "CreateSamplerState for render target failed");
        YAGET_SET_DEBUG_NAME(mSamplerSate.Get(), mName);
    }

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

    HRESULT hr = d3dDevice->CreateTexture2D(&depthStencilBufferDesc, nullptr, &mDepthStencilBuffer);
    error_handlers::ThrowOnError(hr, "Could not initialize Depth & Stencil Buffers");
    YAGET_SET_DEBUG_NAME(mDepthStencilBuffer.Get(), mName);

    hr = d3dDevice->CreateDepthStencilView(mDepthStencilBuffer.Get(), nullptr, &mDepthStencilView);
    error_handlers::ThrowOnError(hr, "Could not create Depth & Stencil view");
    YAGET_SET_DEBUG_NAME(mDepthStencilView.Get(), mName);

    D3D11_DEPTH_STENCIL_DESC depthStencilStateDesc = {};

    depthStencilStateDesc.DepthEnable = TRUE;
    depthStencilStateDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
    depthStencilStateDesc.DepthFunc = D3D11_COMPARISON_LESS;
    depthStencilStateDesc.StencilEnable = FALSE;

    hr = d3dDevice->CreateDepthStencilState(&depthStencilStateDesc, &mDepthStencilState);
    error_handlers::ThrowOnError(hr, "Could not create Depth & Stencil state");
    YAGET_SET_DEBUG_NAME(mDepthStencilState.Get(), mName);

    D3D11_RASTERIZER_DESC rasterizerDesc = {};

    rasterizerDesc.AntialiasedLineEnable = FALSE;
    rasterizerDesc.CullMode = D3D11_CULL_BACK;
    rasterizerDesc.DepthBias = 0;
    rasterizerDesc.DepthBiasClamp = 0.0f;
    rasterizerDesc.DepthClipEnable = TRUE;
    rasterizerDesc.FillMode = D3D11_FILL_SOLID;
    rasterizerDesc.FrontCounterClockwise = FALSE;
    rasterizerDesc.MultisampleEnable = FALSE;
    rasterizerDesc.ScissorEnable = FALSE;
    rasterizerDesc.SlopeScaledDepthBias = 0.0f;

    // Create the rasterizer wire state object.
    hr = d3dDevice->CreateRasterizerState(&rasterizerDesc, &mDefaultRasterizerState);
    error_handlers::ThrowOnError(hr, "Could not create rasterizer state");
    YAGET_SET_DEBUG_NAME(mDefaultRasterizerState.Get(), mName);

    rasterizerDesc.FillMode = D3D11_FILL_WIREFRAME;
    rasterizerDesc.CullMode = D3D11_CULL_NONE;
    hr = d3dDevice->CreateRasterizerState(&rasterizerDesc, &mWireRasterizerState);
    error_handlers::ThrowOnError(hr, "Could not create wire rasterizer state");
    YAGET_SET_DEBUG_NAME(mWireRasterizerState.Get(), mName);
}
