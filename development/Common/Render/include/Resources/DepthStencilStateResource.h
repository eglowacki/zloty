//////////////////////////////////////////////////////////////////////
// DepthStencilStateResource.h
//
//  Copyright 8/11/2019 Edgar Glowacki
//
//  Maintained by: Edgar
//
//  NOTES:
//      
//
//
//  #include "Resources/DepthStencilStateResource.h"
//
//////////////////////////////////////////////////////////////////////
//! \file
#pragma once

#include "YagetCore.h"
#include "Resources/ResourceView.h"
#include <wrl/client.h>

struct ID3D11DepthStencilState;

namespace yaget
{
    namespace io::render { class DepthStencilStateAsset; }

    namespace render::state
    {

        class DepthStencilStateResource : public ResourceView
        {
        public:
            DepthStencilStateResource(Device& device, std::shared_ptr<io::render::DepthStencilStateAsset> asset);

            bool Activate() override;
            void UpdateGui(comp::Component::UpdateGuiType updateGuiType) override;
            const char* GetNameType() const override { return "Depth-Stencil State"; }

        private:
            Microsoft::WRL::ComPtr<ID3D11DepthStencilState> mState;
        };


    } // namespace render::state

} // namespace yaget
