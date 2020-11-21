#include "Resources/RasterizerStateResource.h"
#include "VTS/RenderStateResolvedAssets.h"
#include "Device.h"
#include "RenderHelpers.h"
#include "HashUtilities.h"
#include <d3d11.h>



yaget::render::state::RasterizerStateResource::RasterizerStateResource(Device& device, std::shared_ptr<io::render::RasterizerStateAsset> asset)
    : ResourceView(device, asset->mTag, std::type_index(typeid(RasterizerStateResource)))
{
    Device::ID3D11Device_t* hardwareDevice = mDevice.GetDevice();

    D3D11_RASTERIZER_DESC rasterizerDesc = {};

    rasterizerDesc.AntialiasedLineEnable = asset->mAntialiasedLineEnable;
    rasterizerDesc.CullMode = asset->mCullMode;
    rasterizerDesc.DepthBias = asset->mDepthBias;
    rasterizerDesc.DepthBiasClamp = asset->mDepthBiasClamp;
    rasterizerDesc.DepthClipEnable = asset->mDepthClipEnable;
    rasterizerDesc.FillMode = asset->mFillMode;
    rasterizerDesc.FrontCounterClockwise = asset->mFrontCounterClockwise;
    rasterizerDesc.MultisampleEnable = asset->mMultisampleEnable;
    rasterizerDesc.ScissorEnable = asset->mScissorEnable;
    rasterizerDesc.SlopeScaledDepthBias = asset->mSlopeScaledDepthBias;

    // Create the rasterizer wire state object.
    HRESULT hr = hardwareDevice->CreateRasterizerState(&rasterizerDesc, &mState);
    YAGET_UTIL_THROW_ON_RROR(hr, "Could not create rasterizer state");
    YAGET_SET_DEBUG_NAME(mState.Get(), asset->mTag.mName);

    std::size_t hashValue = conv::GenerateHash(rasterizerDesc);
    SetHashValue(hashValue);
    SetPlatformResource(mState.Get());
}


bool yaget::render::state::RasterizerStateResource::Activate()
{
    mDevice.ActivatedResource(this, fmt::format("This: {}, Hash: {}", static_cast<void*>(this), GetStateHash()).c_str());

    Device::ID3D11DeviceContext_t* deviceContext = mDevice.GetDeviceContext();

    deviceContext->RSSetState(mState.Get());
    return true;
}


void yaget::render::state::RasterizerStateResource::UpdateGui(comp::Component::UpdateGuiType updateGuiType)
{
    using Section = io::VirtualTransportSystem::Section;

    if (updateGuiType == comp::Component::UpdateGuiType::Default)
    {
    }
}

