/////////////////////////////////////////////////////////////////////////
// TextureResource.h
//
//  Copyright 2/26/2017 Edgar Glowacki.
//
// NOTES:
//      Wrapper for texture resource
//
//
// #include "TextureResource.h"
//
/////////////////////////////////////////////////////////////////////////
//! \file
#pragma once

#include "YagetCore.h"
#include "Resources/ResourceView.h"
#include "ImageLoaders/ImageProcessor.h"
#include <wrl/client.h>

struct ID3D11ShaderResourceView;
struct ID3D11SamplerState;

namespace yaget
{
    namespace io { class ImageAsset; }
    namespace io::render { class TextureAsset; class ImageMetaAsset; }

    namespace render
    {
        //namespace io { class TextureAsset; }
        class Device;
        class RenderTarget;
        class TextureImageResource;
        class TextureMetaResource;
        

        // Any texture/image used by shader as a sampler
        class TextureResource : public ResourceView
        {
        public:
            TextureResource(Device& device, std::shared_ptr<io::render::TextureAsset> asset);
            ~TextureResource();

            bool Activate() override;
            void UpdateGui(comp::Component::UpdateGuiType updateGuiType) override;
            const char* GetNameType() const override { return "Texture"; }

        private:
            mt::SmartVariable<render::TextureImageResource> mTextureView;
            mt::SmartVariable<render::TextureMetaResource> mSampler;

            // used for hot reloading of assets
            uint64_t mRefreshTextureViewId;
            uint64_t mRefreshSamplerId;
        };

        // Any texture/image used by shader as a sampler
        class TextureImageResource : public ResourceView
        {
        public:
            TextureImageResource(Device& device, std::shared_ptr<io::ImageAsset> asset);

            bool Activate() override;
            void UpdateGui(comp::Component::UpdateGuiType updateGuiType) override;
            const char* GetNameType() const override { return "Texture Image"; }

        private:
            const image::Header mImageHeader;
            Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> mTextureView;
        };

        // Properties for TextureResource texture/image
        class TextureMetaResource : public ResourceView
        {
        public:
            TextureMetaResource(Device& device, std::shared_ptr<io::render::ImageMetaAsset> asset);

            bool Activate() override;
            const char* GetNameType() const override { return "Texture Meta"; }

        private:
            Microsoft::WRL::ComPtr<ID3D11SamplerState> mSamplerState;
        };

    } // namespace render
} // namespace yaget

