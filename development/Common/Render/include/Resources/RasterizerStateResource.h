//////////////////////////////////////////////////////////////////////
// RasterizerStateResource.h
//
//  Copyright 8/11/2019 Edgar Glowacki
//
//  Maintained by: Edgar
//
//  NOTES:
//      
//
//
//  #include "Resources/RasterizerStateResource.h"
//
//////////////////////////////////////////////////////////////////////
//! \file
#pragma once

#include "YagetCore.h"
#include "Resources/ResourceView.h"
#include <wrl/client.h>

struct ID3D11RasterizerState;

namespace yaget
{
    namespace io::render { class RasterizerStateAsset; }

    namespace render::state
    {

        class RasterizerStateResource : public ResourceView
        {
        public:
            RasterizerStateResource(Device& device, std::shared_ptr<io::render::RasterizerStateAsset> asset);

            bool Activate() override;
            void UpdateGui(comp::Component::UpdateGuiType updateGuiType) override;
            const char* GetNameType() const override { return "Rasterizer State"; }

        private:
            Microsoft::WRL::ComPtr<ID3D11RasterizerState> mState;
        };

    } // namespace render::state
} // namespace yaget
