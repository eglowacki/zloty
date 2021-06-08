//////////////////////////////////////////////////////////////////////
// DescriptionResource.h
//
//  Copyright 7/28/2019 Edgar Glowacki
//
//  Maintained by: Edgar
//
//  NOTES:
//      Description for geometry, material and textures which compose an renderable object
//
//
//  #include "Resources/DescriptionResource.h"
//
//////////////////////////////////////////////////////////////////////
//! \file
#pragma once

#include "YagetCore.h"
#include "Resources/ResourceView.h"
#include "VTS/RenderResolvedAssets.h"
#include "ShaderResources.h"
#include "RenderMathFacade.h"


namespace yaget
{
    namespace io { namespace render {class GeomAsset; class PakAsset; class DescriptionAsset; }}

    namespace render
    {
        namespace state { class DepthStencilStateResource; class RasterizerStateResource; class BlendStateResource; }
        class SkinShaderResource;
        class GeometryResource;
        class TextureResource;

        class DescriptionResource : public ResourceView
        {
        public:
            DescriptionResource(Device& device, std::shared_ptr<io::render::DescriptionAsset> asset);

            bool Activate() override;
            bool Activate(const Strings& passes);
            void UpdateGui(comp::Component::UpdateGuiType updateGuiType) override;
            const char* GetNameType() const override { return "Description"; }

            const io::render::Layers mLayers;

            struct Pass
            {
                Pass(Device& device, const yaget::io::Tag& materialTag, const yaget::io::Tag& geometryTag, const yaget::io::Tag& textureTag,
                    const yaget::io::Tag& depthStateTag, const yaget::io::Tag& rasterizerStateTag, const yaget::io::Tag& blendStateTag);
                ~Pass();

                Device& mDevice;

                mt::SmartVariable<render::SkinShaderResource> mSkinShader;
                mt::SmartVariable<render::GeometryResource> mGeometry;
                mt::SmartVariable<render::TextureResource> mTexture;
                mt::SmartVariable<render::state::DepthStencilStateResource> mDepthState;
                mt::SmartVariable<render::state::RasterizerStateResource> mRasterizerState;
                mt::SmartVariable<render::state::BlendStateResource> mBlendState;

                uint64_t mRefreshSkinShaderId = 0;
                uint64_t mRefreshGeometryId = 0;
                uint64_t mRefreshTextureId = 0;
                uint64_t mRefreshDepthStateId = 0;
                uint64_t mRefreshRasterizerStateId = 0;
                uint64_t mRefreshBlendStateId = 0;
                bool mActive = false;
            };

        private:
            using PassValue = std::unique_ptr<Pass>;
            using Passes = std::unordered_map<std::string, PassValue>;
            Passes mPasses;
            bool mGlobalPassUsage = true;
            Strings mLastRequestedPasses;
        };

    } // namespace render
} // namespace yaget