//////////////////////////////////////////////////////////////////////
// BlendStateResource.h
//
//  Copyright 8/11/2019 Edgar Glowacki
//
//  Maintained by: Edgar
//
//  NOTES:
//      
//
//
//  #include "Resources/BlendStateResource.h"
//
//////////////////////////////////////////////////////////////////////
//! \file
#pragma once

#include "YagetCore.h"
#include "Resources/ResourceView.h"
#include "MathFacade.h"
#include <wrl/client.h>

struct ID3D11BlendState;

namespace yaget
{
    namespace io::render { class BlendStateAsset; }

    namespace render::state
    {

        class BlendStateResource : public ResourceView
        {
        public:
            BlendStateResource(Device& device, std::shared_ptr<io::render::BlendStateAsset> asset);

            bool Activate() override;
            bool Activate(const colors::Color& blendFactor);
            void UpdateGui(comp::Component::UpdateGuiType updateGuiType) override;
            const char* GetNameType() const override { return "Blend State"; }

        private:
            Microsoft::WRL::ComPtr<ID3D11BlendState> mState;
            math3d::Color mBlendFactor;
        };


    } // namespace render::state

} // namespace yaget
