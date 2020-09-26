#include "Resources/DepthStencilStateResource.h"
#include "VTS/RenderStateResolvedAssets.h"
#include "Device.h"
#include "RenderHelpers.h"
#include "HashUtilities.h"
#include <d3d11.h>


// DepthEnable, 
// D3D11_DEPTH_WRITE_MASK, 
// D3D11_COMPARISON_FUNC, 
// StencilEnable, 
// StencilReadMask
// StencilWriteMask
// NOT Supporting D3D11_DEPTH_STENCILOP_DESC flag

yaget::render::state::DepthStencilStateResource::DepthStencilStateResource(Device& device, std::shared_ptr<io::render::DepthStencilStateAsset> asset)
    : ResourceView(device, asset->mTag, std::type_index(typeid(DepthStencilStateResource)))
{
    Device::ID3D11Device_t* hardwareDevice = mDevice.GetDevice();

    D3D11_DEPTH_STENCIL_DESC stateDesc = {};

    stateDesc.DepthEnable = asset->mDepthEnable ? TRUE : FALSE;
    stateDesc.DepthWriteMask = asset->mDepthWriteMask;//  D3D11_DEPTH_WRITE_MASK_ALL;
    stateDesc.DepthFunc = asset->mDepthComparisonFunc;//   D3D11_COMPARISON_LESS_EQUAL;// D3D11_COMPARISON_LESS
    stateDesc.StencilEnable = asset->mStencilEnable;//  FALSE;
    stateDesc.StencilReadMask = asset->mStencilReadMask;
    stateDesc.StencilWriteMask = asset->mStencilWriteMask;

    HRESULT hr = hardwareDevice->CreateDepthStencilState(&stateDesc, &mState);
    YAGET_THROW_ON_RROR(hr, "Could not create Depth & Stencil state");
    YAGET_SET_DEBUG_NAME(mState.Get(), asset->mTag.mName);

    std::size_t hashValue = conv::GenerateHash(stateDesc);
    SetHashValue(hashValue);
    SetPlatformResource(mState.Get());
}

bool yaget::render::state::DepthStencilStateResource::Activate()
{
    mDevice.ActivatedResource(this, fmt::format("This: {}, Hash: {}", static_cast<void*>(this), GetStateHash()).c_str());

    Device::ID3D11DeviceContext_t* deviceContext = mDevice.GetDeviceContext();

    deviceContext->OMSetDepthStencilState(mState.Get(), 1);
    return true;
}

void yaget::render::state::DepthStencilStateResource::UpdateGui(comp::Component::UpdateGuiType updateGuiType)
{
    using Section = io::VirtualTransportSystem::Section;

    if (updateGuiType == comp::Component::UpdateGuiType::Default)
    {
    }
}
