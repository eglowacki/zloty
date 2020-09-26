#include "TextureResource.h"
#include "App/Application.h"
#include "Device.h"
#include "RenderHelpers.h"
#include "Debugging/DevConfiguration.h"
#include "ImageLoaders/ImageProcessor.h"
#include "Resources/RenderTargetResource.h"
#include "imgui.h"
#include "Gui/Support.h"

#include <DDSTextureLoader.h>
#include <DirectXTex.h>


//--------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------
yaget::render::TextureImageResource::TextureImageResource(Device& device, std::shared_ptr<io::ImageAsset> asset)
    : ResourceView(device, asset->mTag, std::type_index(typeid(TextureImageResource)))
    , mImageHeader(asset->mHeader)
{
    Device::ID3D11Device_t* hardwareDevice = mDevice.GetDevice();

    if (mImageHeader.mDataType == image::Header::DataType::RT)
    {
        auto resourceView = mDevice.RequestResourceView<RenderTargetResource>(asset->mTag);
        auto* textureMap = resourceView->GetPlatformResource<ID3D11Texture2D>();

        /////////////////////// Map's Shader Resource View
        // Setup the description of the shader resource view.
        D3D11_SHADER_RESOURCE_VIEW_DESC shaderResourceViewDesc = {};
        shaderResourceViewDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        shaderResourceViewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
        shaderResourceViewDesc.Texture2D.MostDetailedMip = 0;
        shaderResourceViewDesc.Texture2D.MipLevels = 1;

        // Create the shader resource view.
        HRESULT hr = hardwareDevice->CreateShaderResourceView(textureMap, &shaderResourceViewDesc, &mTextureView);
        YAGET_THROW_ON_RROR(hr, "CreateShaderResourceView for render target failed");
    }
    else
    {
        DXGI_FORMAT colorFormat = DXGI_FORMAT_UNKNOWN;
        switch (mImageHeader.mColorType)
        {
        case image::Header::PixelType::RGBA:
            colorFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
            break;

        case image::Header::PixelType::Single:
            colorFormat = DXGI_FORMAT_A8_UNORM;
            break;

        default:
            YAGET_ASSERT(false, "header.mColorType '%d' is not supported.", mImageHeader.mColorType);
        }

        if (mImageHeader.mDataType == image::Header::DataType::RAW)
        {
            D3D11_TEXTURE2D_DESC desc = {};
            desc.Width = mImageHeader.mSize.first;
            desc.Height = mImageHeader.mSize.second;
            desc.MipLevels = desc.ArraySize = 1;
            desc.Format = colorFormat;
            desc.SampleDesc.Count = 1;
            desc.Usage = D3D11_USAGE_DYNAMIC;
            desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
            desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
            //desc.MiscFlags = 0;

            const io::Buffer& buffer = asset->mPixels;
            D3D11_SUBRESOURCE_DATA data = { buffer.first.get(), mImageHeader.mSize.first * mImageHeader.PixelSize(), 0 };

            Microsoft::WRL::ComPtr<ID3D11Texture2D> texture;
            HRESULT hr = hardwareDevice->CreateTexture2D(&desc, &data, texture.GetAddressOf());
            YAGET_THROW_ON_RROR(hr, "Could not create 2D texture.");

            hr = hardwareDevice->CreateShaderResourceView(texture.Get(), nullptr, mTextureView.GetAddressOf());
            YAGET_THROW_ON_RROR(hr, "Could not create texture view.");
        }
        else if (mImageHeader.mDataType == image::Header::DataType::DDS)
        {
            const io::Buffer& buffer = asset->mPixels;
            HRESULT hr = DirectX::CreateDDSTextureFromMemory(hardwareDevice, buffer.first.get(), buffer.second, nullptr, mTextureView.GetAddressOf());
            YAGET_THROW_ON_RROR(hr, "Could not create texture view");
        }
        else
        {
            YAGET_ASSERT(false, "header.mDataType '%d' is not supported.", mImageHeader.mDataType);
        }
    }

    YAGET_SET_DEBUG_NAME(mTextureView.Get(), asset->mTag.mName);

    std::size_t hashValue = asset->mTag.Hash();
    SetHashValue(hashValue);
    SetPlatformResource(mTextureView.Get());
}

//--------------------------------------------------------------------------------------------------
bool yaget::render::TextureImageResource::Activate()
{
    mDevice.ActivatedResource(this, fmt::format("This: {}, Hash: {}", static_cast<void*>(this), GetStateHash()).c_str());

    Device::ID3D11DeviceContext_t* d3dDeviceContext = mDevice.GetDeviceContext();

    d3dDeviceContext->PSSetShaderResources(0, 1, mTextureView.GetAddressOf());

    return true;
}

//--------------------------------------------------------------------------------------------------
void yaget::render::TextureImageResource::UpdateGui(comp::Component::UpdateGuiType updateGuiType)
{
    if (updateGuiType == comp::Component::UpdateGuiType::Default)
    {
        auto keyColor = dev::CurrentConfiguration().mGuiColors.at("KeyText");
        auto valueColor = dev::CurrentConfiguration().mGuiColors.at("ValueText");
        yaget::gui::TextColors(keyColor, "Dimensions: ", valueColor, mImageHeader.mSize);
        yaget::gui::TextColors(keyColor, "Mip Maps: ", valueColor, mImageHeader.mNumMipMaps);

        const char* colorType[] = {"?", "Alpha", "Alpha Luminance", "RGB", "RGBA"};
        yaget::gui::TextColors(keyColor, "Color Type: ", valueColor, colorType[mImageHeader.mNumComponents]);

        const float ImageSize = 256.0f;
        ImGui::Image(mTextureView.Get(), ImVec2(ImageSize, ImageSize), ImVec2(0, 0), ImVec2(1, 1), ImColor(255, 255, 255, 255), ImColor(255, 255, 255, 128));
    }
}

//--------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------
yaget::render::TextureMetaResource::TextureMetaResource(Device& device, std::shared_ptr<io::render::ImageMetaAsset> asset)
    : ResourceView(device, asset->mTag, std::type_index(typeid(TextureMetaResource)))
{
    Device::ID3D11Device_t* hardwareDevice = mDevice.GetDevice();
    D3D11_SAMPLER_DESC sd = {};

    sd.AddressU = static_cast<D3D11_TEXTURE_ADDRESS_MODE>(asset->mAddressU);
    sd.AddressV = static_cast<D3D11_TEXTURE_ADDRESS_MODE>(asset->mAddressV);
    sd.AddressW = static_cast<D3D11_TEXTURE_ADDRESS_MODE>(asset->mAddressW);
    sd.ComparisonFunc = static_cast<D3D11_COMPARISON_FUNC>(asset->mComparisonFunc);
    sd.Filter = static_cast<D3D11_FILTER>(asset->mFilter);

    sd.MipLODBias = std::clamp(asset->mMipLODBias, 0.0f, D3D11_FLOAT32_MAX);
    sd.MinLOD = std::clamp(asset->mMinLOD, 0.0f, D3D11_FLOAT32_MAX);
    sd.MaxLOD = std::clamp(asset->mMaxLOD, sd.MinLOD, D3D11_FLOAT32_MAX);

    HRESULT hr = hardwareDevice->CreateSamplerState(&sd, &mSamplerState);
    YAGET_THROW_ON_RROR(hr, "Could not create sampler state for texture");

    YAGET_SET_DEBUG_NAME(mSamplerState.Get(), asset->mTag.mName);

    std::size_t hashValue = asset->mTag.Hash();
    SetHashValue(hashValue);
    SetPlatformResource(mSamplerState.Get());
}

//--------------------------------------------------------------------------------------------------
bool yaget::render::TextureMetaResource::Activate()
{
    mDevice.ActivatedResource(this, fmt::format("This: {}, Hash: {}", static_cast<void*>(this), GetStateHash()).c_str());

    Device::ID3D11DeviceContext_t* d3dDeviceContext = mDevice.GetDeviceContext();

    d3dDeviceContext->PSSetSamplers(0, 1, mSamplerState.GetAddressOf());

    return true;
}

//--------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------
yaget::render::TextureResource::TextureResource(Device& device, std::shared_ptr<io::render::TextureAsset> asset)
    : ResourceView(device, asset->mTag, std::type_index(typeid(TextureResource)))
    , mRefreshTextureViewId(idspace::get_burnable(mDevice.App().IdCache))
    , mRefreshSamplerId(idspace::get_burnable(mDevice.App().IdCache))
{
    mDevice.RequestResourceView<render::TextureImageResource>(asset->mImageTag, std::ref(mTextureView), mRefreshTextureViewId);
    mDevice.RequestResourceView<render::TextureMetaResource>(asset->mMetaTag, std::ref(mSampler), mRefreshSamplerId);

    std::size_t hashValue = asset->mTag.Hash();
    SetHashValue(hashValue);
}

//--------------------------------------------------------------------------------------------------
yaget::render::TextureResource::~TextureResource()
{
    mDevice.RemoveWatch(mRefreshTextureViewId);
    mDevice.RemoveWatch(mRefreshSamplerId);
}

//--------------------------------------------------------------------------------------------------
bool yaget::render::TextureResource::Activate()
{
    mDevice.ActivatedResource(this, fmt::format("This: {}, Hash: {}", static_cast<void*>(this), GetStateHash()).c_str());

    mt::SmartVariable<render::TextureImageResource>::SmartType textureView = mTextureView;
    mt::SmartVariable<render::TextureMetaResource>::SmartType sampler = mSampler;

    return sampler && textureView && sampler->Activate() && textureView->Activate();
}

void yaget::render::TextureResource::UpdateGui(comp::Component::UpdateGuiType updateGuiType)
{
    using Section = io::VirtualTransportSystem::Section;

    if (updateGuiType == comp::Component::UpdateGuiType::Default)
    {
        mt::SmartVariable<render::TextureImageResource>::SmartType textureView = mTextureView;
        mt::SmartVariable<render::TextureMetaResource>::SmartType sampler = mSampler;

        math3d::Color tagColor(dev::CurrentConfiguration().mGuiColors.at("SectionText"));
        if (textureView)
        {
            gui::UpdateSectionText("TextureView, Tag:", tagColor, textureView.get(), updateGuiType);
        }
        if (sampler)
        {
            gui::UpdateSectionText("Sampler, Tag:", tagColor, sampler.get(), updateGuiType);
        }
    }
}
