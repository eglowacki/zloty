#include "Resources/BlendStateResource.h"
#include "VTS/RenderStateResolvedAssets.h"
#include "Device.h"
#include "RenderHelpers.h"
#include "App/AppUtilities.h"
#include "HashUtilities.h"
#include "imgui.h"
#include "Gui/Support.h"
#include <d3d11.h>


namespace
{
    const std::size_t kMaxBendableTargets = 8;

} // namespace


yaget::render::state::BlendStateResource::BlendStateResource(Device& device, std::shared_ptr<io::render::BlendStateAsset> asset)
    : ResourceView(device, asset->mTag, std::type_index(typeid(BlendStateResource)))
{
    YAGET_UTIL_THROW_ASSERT("REND", asset->mTargetBlends.size() < kMaxBendableTargets, fmt::format("BlendStateAsset has: '{}' blend targets, but only maximum of '{}' is supported.", asset->mTargetBlends.size(), kMaxBendableTargets));

    Device::ID3D11Device_t* hardwareDevice = mDevice.GetDevice();

    mBlendFactor = asset->mBlendFactor;

    D3D11_BLEND_DESC blendDesc = {};
    blendDesc.AlphaToCoverageEnable = asset->mAlphaToCoverageEnable;
    blendDesc.IndependentBlendEnable = asset->mIndependentBlendEnable;

    std::size_t index = 0;
    for (const auto& blend : asset->mTargetBlends)
    {
        blendDesc.RenderTarget[index].BlendEnable = blend.mBlendEnable;
        blendDesc.RenderTarget[index].SrcBlend = blend.mSrcBlend;
        blendDesc.RenderTarget[index].DestBlend = blend.mDestBlend;
        blendDesc.RenderTarget[index].BlendOp = blend.mBlendOp;
        blendDesc.RenderTarget[index].SrcBlendAlpha = blend.mSrcBlendAlpha;
        blendDesc.RenderTarget[index].DestBlendAlpha = blend.mDestBlendAlpha;
        blendDesc.RenderTarget[index].BlendOpAlpha = blend.mBlendOpAlpha;
        blendDesc.RenderTarget[index].RenderTargetWriteMask = blend.mRenderTargetWriteMask;

        ++index;
    }

    HRESULT hr = hardwareDevice->CreateBlendState(&blendDesc, &mState);
    YAGET_UTIL_THROW_ON_RROR(hr, "Could not create blender state");
    YAGET_SET_DEBUG_NAME(mState.Get(), asset->mTag.mName);

    std::size_t hashValue = conv::GenerateHash(blendDesc, mBlendFactor);
    SetHashValue(hashValue);
    SetPlatformResource(mState.Get());
}

bool yaget::render::state::BlendStateResource::Activate(const colors::Color& blendFactor)
{
    
    
    mDevice.ActivatedResource(this, fmt::format("This: {}, Hash: {}", static_cast<void*>(this), GetStateHash()).c_str());

    Device::ID3D11DeviceContext_t* deviceContext = mDevice.GetDeviceContext();

    deviceContext->OMSetBlendState(mState.Get(), mBlendFactor * blendFactor, 0XFFFFFFFF);
    return true;
}

bool yaget::render::state::BlendStateResource::Activate()
{
    return Activate(colors::White);
}

void yaget::render::state::BlendStateResource::UpdateGui(comp::Component::UpdateGuiType updateGuiType)
{
    using Section = io::VirtualTransportSystem::Section;

    if (updateGuiType == comp::Component::UpdateGuiType::Default)
    {
        ImGui::Text("Blend Factor: ");
        ImGui::SameLine();

        yaget::gui::ColorEdit4("Color", mBlendFactor);

        ImGui::NewLine();
    }
}
