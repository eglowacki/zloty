//////////////////////////////////////////////////////////////////////
// SkinShaderResource.h
//
//  Copyright 7/25/2018 Edgar Glowacki
//
//  Maintained by: Edgar
//
//  NOTES:
//      Collection of vertex, pixel, input layout, textures, and other state to activate for a specific render effect (skin)
//
//
//  #include "Resources/SkinShaderResource.h"
//
//////////////////////////////////////////////////////////////////////
//! \file
#pragma once

#include "YagetCore.h"
#include "Resources/ResourceView.h"
#include "ThreadModel/Variables.h"
#include "TextureResource.h"


namespace yaget
{
    namespace io::render { class MaterialAsset; }

    namespace render
    {
        class Device;
        class VertexShaderResource;
        class PixelShaderResource;
        class InputLayoutResource;
        class ConstantsResource;

        //--------------------------------------------------------------------------------------------------
        class SkinShaderResource : public ResourceView
        {
        public:
            SkinShaderResource(Device& device, std::shared_ptr<io::render::MaterialAsset> asset);
            ~SkinShaderResource();

            bool Activate() override;
            void UpdateGui(comp::Component::UpdateGuiType updateGuiType) override;
            const char* GetNameType() const override { return "Skin Shader"; }

        private:
            uint64_t mRefreshVertexId;
            uint64_t mRefreshPixelId;

            mt::SmartVariable<VertexShaderResource> mVertex;
            mt::SmartVariable<PixelShaderResource> mPixel;
            mt::SmartVariable<InputLayoutResource> mInput;

            mt::SmartVariable<ConstantsResource> mConstantBuffer;
        };

    } // namespace render
} // namespace yaget
