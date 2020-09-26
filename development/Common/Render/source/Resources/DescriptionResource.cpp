#include "Resources/DescriptionResource.h"
#include "VTS/RenderResolvedAssets.h"
#include "Device.h"
#include "IdGameCache.h"
#include "App/Application.h"
#include "Debugging/DevConfiguration.h"
#include "Resources/SkinShaderResource.h"
#include "Resources/GeometryResource.h"
#include "Resources/DepthStencilStateResource.h"
#include "Resources/RasterizerStateResource.h"
#include "Resources/BlendStateResource.h"
#include "Resources/RenderStateCache.h"
#include "TextureResource.h"
#include "imgui.h"
#include "Gui/Support.h"

namespace
{
    using namespace  yaget;

    struct PassHolder
    {
        // if resourceLayer has valid Tag for a specific channel, assign it from pass
        void Accumulate(const io::render::Layer& resourceLayer, const render::DescriptionResource::Pass& pass)
        {
            if (resourceLayer.mMaterialTag.IsValid())
            {
                mShaderValid = true;
                mShader = pass.mSkinShader;
            }
            if (resourceLayer.mGeometryTag.IsValid())
            {
                mGeometryValid = true;
                mGeometry = pass.mGeometry;
            }
            if (resourceLayer.mTextureTag.IsValid())
            {
                mTextureValid = true;
                mTexture = pass.mTexture;
            }
            if (resourceLayer.mDepthStateTag.IsValid())
            {
                mDepthStateValid = true;
                mDepthState = pass.mDepthState;
            }
            if (resourceLayer.mRasterizerStateTag.IsValid())
            {
                mRasterizerStateValid = true;
                mRasterizerState = pass.mRasterizerState;
            }
            if (resourceLayer.mBlendStateTag.IsValid())
            {
                mBlendStateValid = true;
                mBlendState = pass.mBlendState;
            }
        }

        // assign values from pass for each channel only if previousPassHolder did not use that channel
        void Accumulate(const PassHolder& previousPassHolder, const render::DescriptionResource::Pass& pass)
        {
            if (!previousPassHolder.mShaderValid)
            {
                mShaderValid = true;
                mShader = pass.mSkinShader;
            }
            if (!previousPassHolder.mGeometryValid)
            {
                mGeometryValid = true;
                mGeometry = pass.mGeometry;
            }
            if (!previousPassHolder.mTextureValid)
            {
                mTextureValid = true;
                mTexture = pass.mTexture;
            }
            if (!previousPassHolder.mDepthStateValid)
            {
                mDepthStateValid = true;
                mDepthState = pass.mDepthState;
            }
            if (!previousPassHolder.mRasterizerState)
            {
                mRasterizerStateValid = true;
                mRasterizerState = pass.mRasterizerState;
            }
            if (!previousPassHolder.mBlendState)
            {
                mBlendStateValid = true;
                mBlendState = pass.mBlendState;
            }
        }

        // Replace any channel with previousPassHolder value if it used by previousPassHolder
        void Accumulate(const PassHolder& previousPassHolder)
        {
            if (previousPassHolder.mShaderValid)
            {
                mShaderValid = true;
                mShader = previousPassHolder.mShader;
            }
            if (previousPassHolder.mGeometryValid)
            {
                mGeometryValid = true;
                mGeometry = previousPassHolder.mGeometry;
            }
            if (previousPassHolder.mTextureValid)
            {
                mTextureValid = true;
                mTexture = previousPassHolder.mTexture;
            }
            if (previousPassHolder.mDepthStateValid)
            {
                mDepthStateValid = true;
                mDepthState = previousPassHolder.mDepthState;
            }
            if (previousPassHolder.mRasterizerState)
            {
                mRasterizerStateValid = true;
                mRasterizerState = previousPassHolder.mRasterizerState;
            }
            if (previousPassHolder.mBlendState)
            {
                mBlendStateValid = true;
                mBlendState = previousPassHolder.mBlendState;
            }
        }

        bool Activate(render::Device& device)
        {
            if (mShaderValid && mShader && mGeometryValid && mGeometry)
            {
                render::state::ResourceActivator& resourceActivator = device.ResourceActivator();

                render::state::RenderStateCache& stateCache = device.StateCache();
                render::state::DefaultStateCache defaultStateCache(stateCache);

                SetResourceState(mDepthState, mDepthStateValid, stateCache, resourceActivator);
                SetResourceState(mRasterizerState, mRasterizerStateValid, stateCache, resourceActivator);
                SetResourceState(mBlendState, mBlendStateValid, stateCache, resourceActivator);

                if (!stateCache.Activate())
                {
                    return false;
                }

                if (mTextureValid)
                {
                    if (mTexture)
                    {
                        return mTexture->Activate() && mShader->Activate() && mGeometry->Activate();
                    }

                    return false;
                }

                return mShader->Activate() && mGeometry->Activate();
            }

            return false;
        }

    private:
        template<typename T>
        bool SetResourceState(T& stateResource, bool stateValid, render::state::RenderStateCache& stateCache, render::state::ResourceActivator& /*resourceActivator*/)
        {
            if (stateValid && stateResource)
            {
                stateCache.Set(stateResource, render::state::RenderStateCache::SetPolicy::AutoReset);

                //resourceActivator.Set(stateResource);
                return true;
            }
            else if (stateValid)
            {
                return false;
            }

            return true;
        }

        mt::SmartVariable<render::SkinShaderResource>::SmartType mShader;
        mt::SmartVariable<render::GeometryResource>::SmartType mGeometry;
        mt::SmartVariable<render::TextureResource>::SmartType mTexture;
        mt::SmartVariable<render::state::DepthStencilStateResource>::SmartType mDepthState;
        mt::SmartVariable<render::state::RasterizerStateResource>::SmartType mRasterizerState;
        mt::SmartVariable<render::state::BlendStateResource>::SmartType mBlendState;
        bool mShaderValid = false;
        bool mGeometryValid = false;
        bool mTextureValid = false;
        bool mDepthStateValid = false;
        bool mRasterizerStateValid = false;
        bool mBlendStateValid = false;
    };

} // namespace


yaget::render::DescriptionResource::Pass::Pass(Device& device, const yaget::io::Tag& materialTag, const yaget::io::Tag& geometryTag, const yaget::io::Tag& textureTag, const yaget::io::Tag& depthStateTag, const yaget::io::Tag& rasterizerStateTag, const yaget::io::Tag& blendStateTag)
    : mDevice(device)
{
    if (materialTag.IsValid())
    {
        mRefreshSkinShaderId = idspace::get_burnable(mDevice.App().IdCache);
        mDevice.RequestResourceView<render::SkinShaderResource>(materialTag, std::ref(mSkinShader), mRefreshSkinShaderId);
    }

    if (geometryTag.IsValid())
    {
        mRefreshGeometryId = 0;// idspace::get_burnable(mDevice.App().IdCache);
        mDevice.RequestResourceView<render::GeometryResource>(geometryTag, std::ref(mGeometry), mRefreshGeometryId);
    }

    if (textureTag.IsValid())
    {
        mRefreshTextureId = idspace::get_burnable(mDevice.App().IdCache);
        mDevice.RequestResourceView<render::TextureResource>(textureTag, std::ref(mTexture), mRefreshTextureId);
    }

    if (depthStateTag.IsValid())
    {
        mRefreshDepthStateId = idspace::get_burnable(mDevice.App().IdCache);
        mDevice.RequestResourceView<render::state::DepthStencilStateResource>(depthStateTag, std::ref(mDepthState), mRefreshDepthStateId);
    }

    if (rasterizerStateTag.IsValid())
    {
        mRefreshRasterizerStateId = idspace::get_burnable(mDevice.App().IdCache);
        mDevice.RequestResourceView<render::state::RasterizerStateResource>(rasterizerStateTag, std::ref(mRasterizerState), mRefreshRasterizerStateId);
    }

    if (blendStateTag.IsValid())
    {
        mRefreshBlendStateId = idspace::get_burnable(mDevice.App().IdCache);
        mDevice.RequestResourceView<render::state::BlendStateResource>(blendStateTag, std::ref(mBlendState), mRefreshBlendStateId);
    }
}


yaget::render::DescriptionResource::Pass::~Pass()
{
    mDevice.RemoveWatch(mRefreshSkinShaderId);
    mDevice.RemoveWatch(mRefreshGeometryId);
    mDevice.RemoveWatch(mRefreshTextureId);
    mDevice.RemoveWatch(mRefreshDepthStateId);
    mDevice.RemoveWatch(mRefreshRasterizerStateId);
    mDevice.RemoveWatch(mRefreshBlendStateId);
}

yaget::render::DescriptionResource::DescriptionResource(Device& device, std::shared_ptr<yaget::io::render::DescriptionAsset> asset)
    : ResourceView(device, asset->mTag, std::type_index(typeid(DescriptionResource)))
    , mLayers(asset->mLayers)
{
    for (const auto& it : mLayers)
    {
        mPasses.insert({ it.first, std::make_unique<Pass>(mDevice, it.second.mMaterialTag, it.second.mGeometryTag, it.second.mTextureTag, it.second.mDepthStateTag, it.second.mRasterizerStateTag, it.second.mBlendStateTag) });
    }

    std::size_t hashValue = asset->mTag.Hash();
    SetHashValue(hashValue);
}


bool yaget::render::DescriptionResource::Activate()
{
    return Activate({ "Default" });
}


void yaget::render::DescriptionResource::UpdateGui(comp::Component::UpdateGuiType updateGuiType)
{
    using Section = io::VirtualTransportSystem::Section;

    if (updateGuiType == comp::Component::UpdateGuiType::Default)
    {
        int counter = 1;

        bool result = ImGui::RadioButton(fmt::format("Global##{}", counter++).c_str(), mGlobalPassUsage);
        yaget::gui::SetTooltip("Use global render list for passes");

        ImGui::SameLine();
        if (result)
        {
            mGlobalPassUsage = true;
        }
        else if (ImGui::RadioButton(fmt::format("Override##{}", counter++).c_str(), !mGlobalPassUsage))
        {
            mGlobalPassUsage = false;
        }

        yaget::gui::SetTooltip("Use user selected render list for passes");

        for (const auto& pass : mPasses)
        {
            const PassValue& value = pass.second;

            bool passActive = std::find(std::begin(mLastRequestedPasses), std::end(mLastRequestedPasses), pass.first) != std::end(mLastRequestedPasses);
            bool visible = ImGui::TreeNode(fmt::format("Pass Name: ##{}", pass.first).c_str());
            ImGui::SameLine();

            yaget::gui::MakeDisabled(mGlobalPassUsage, [&counter, &value]()
            {
                ImGui::Checkbox(fmt::format("##{}", counter++).c_str(), &value->mActive);
                ImGui::SameLine();
            });

            if (mGlobalPassUsage)
            {
                yaget::gui::SetTooltip("User pass selection not available. Turn 'Override' radio option on to select which render passes to use");
            }
            else
            {
                yaget::gui::SetTooltip(value->mActive ?
                    "Pass is active in render loop. Click to disable it" :
                    "Pass is not active in render loop. Click to enable it.");
            }

            const math3d::Color passColor = passActive ? dev::CurrentConfiguration().mGuiColors.at("ActiveText") : dev::CurrentConfiguration().mGuiColors.at("InactiveText");
            yaget::gui::Text(fmt::format("'{}'", pass.first), passColor);

            if (visible)
            {
                mt::SmartVariable<render::SkinShaderResource>::SmartType skinShader = value->mSkinShader;
                mt::SmartVariable<render::GeometryResource>::SmartType geometry = value->mGeometry;
                mt::SmartVariable<render::TextureResource>::SmartType texture = value->mTexture;
                mt::SmartVariable<render::state::DepthStencilStateResource>::SmartType depthState = value->mDepthState;
                mt::SmartVariable<render::state::RasterizerStateResource>::SmartType rasterizerState = value->mRasterizerState;
                mt::SmartVariable<render::state::BlendStateResource>::SmartType blendState = value->mBlendState;

                math3d::Color tagColor(dev::CurrentConfiguration().mGuiColors.at("SectionText"));
                if (skinShader)
                {
                    gui::UpdateSectionText("SkinShader, Tag:", tagColor, skinShader.get(), updateGuiType);
                }
                if (geometry)
                {
                    gui::UpdateSectionText("Geometry, Tag:", tagColor, geometry.get(), updateGuiType);
                }
                if (texture)
                {
                    gui::UpdateSectionText("Texture, Tag:", tagColor, texture.get(), updateGuiType);
                }
                if (depthState)
                {
                    gui::UpdateSectionText("DepthState, Tag:", tagColor, depthState.get(), updateGuiType);
                }
                if (rasterizerState)
                {
                    gui::UpdateSectionText("RasterizerState, Tag:", tagColor, rasterizerState.get(), updateGuiType);
                }
                if (blendState)
                {
                    gui::UpdateSectionText("BlendState, Tag:", tagColor, blendState.get(), updateGuiType);
                }

                ImGui::TreePop();
            }
        }
    }
}


////This example illustrates declaring global state block variables.
//BlendState myBS[2] < bool IsValid = true; >
//{
//    {
//        BlendEnable[0] = false;
//    },
//  {
//    BlendEnable[0] = true;
//    SrcBlendAlpha[0] = Inv_Src_Alpha;
//  }
//};
//
//RasterizerState myRS
//{
//      FillMode = Solid;
//      CullMode = NONE;
//      MultisampleEnable = true;
//      DepthClipEnable = false;
//};
//
//DepthStencilState myDS
//{
//    DepthEnable = false;
//    DepthWriteMask = Zero;
//    DepthFunc = Less;
//};
//sampler mySS[2] : register(s3)
//{
//    {
//        Filter = ANISOTROPIC;
//        MaxAnisotropy = 3;
//    },
//    {
//        Filter = ANISOTROPIC;
//        MaxAnisotropy = 4;
//    }
//};
bool yaget::render::DescriptionResource::Activate(const Strings& passes)
{
    mDevice.ActivatedResource(this, fmt::format("This: {}, Hash: {}", static_cast<void*>(this), GetStateHash()).c_str());

    mLastRequestedPasses = passes;
    bool result = false;
    PassHolder passHolder;
    PassHolder overlayPassHolder;

    const io::render::Layer* previousLayer = nullptr;

    for (const auto& passName : passes)
    {
        if (const auto& it_pass = mPasses.find(passName); it_pass != mPasses.end())
        {
            mDevice.ActivatedResource(nullptr, fmt::format("    Active Pass: {}", passName).c_str());

            const io::render::Layer& resourceLayer = mLayers.find(passName)->second;
            const auto& currenttPass = *it_pass->second.get();

            if (resourceLayer.mOperation == io::render::Layer::Operation::Flatten)
            {
                passHolder.Accumulate(resourceLayer, currenttPass);
                previousLayer = &resourceLayer;
                continue;
            }
            else if (resourceLayer.mOperation == io::render::Layer::Operation::Overlay)
            {
                overlayPassHolder.Accumulate(resourceLayer, currenttPass);

                previousLayer = &resourceLayer;
                continue;
            }
            else if (resourceLayer.mOperation == io::render::Layer::Operation::Replace)
            {
                if (previousLayer && previousLayer->mOperation == io::render::Layer::Operation::Flatten)
                {
                    passHolder.Accumulate(passHolder, currenttPass);
                }
                else
                {
                    passHolder.Accumulate(resourceLayer, currenttPass);
                }

                result = passHolder.Activate(mDevice);

                if (result && previousLayer && previousLayer->mOperation == io::render::Layer::Operation::Overlay)
                {
                    passHolder.Accumulate(overlayPassHolder);
                    result = passHolder.Activate(mDevice);
                    overlayPassHolder = {};
                }

                passHolder = {};
                previousLayer = nullptr;
            }
        }
        else
        {
            YLOG_ERROR("RESR", "Pass: '%s' is not in description layers, ignoring.\n%s", passName.c_str(), mAssetTag.ResolveVTS().c_str());
        }
    }

    return result;
}
