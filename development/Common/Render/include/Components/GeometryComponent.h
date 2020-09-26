//////////////////////////////////////////////////////////////////////
// GeometryComponent.h
//
//  Copyright 7/21/2019 Edgar Glowacki
//
//  Maintained by: Edgar
//
//  NOTES:
//      
//
//
//  #include "Components/GeometryComponent.h"
//
//////////////////////////////////////////////////////////////////////
//! \file
#pragma once

#include "YagetCore.h"
#include "Components/RenderComponent.h"
#include "Resources/ResourceWatchRefresher.h"
#include "MathFacade.h"


namespace yaget
{
    namespace render
    {
        class GeometryResource;
        class SkinShaderResource;
        class TextureResource;
        class DescriptionResource;

        class GeometryComponent : public render::RenderComponent
        {
        public:
            GeometryComponent(comp::Id_t id, render::Device& device, const std::vector<io::Tag>& tags);

            void SetMatrix(const math3d::Matrix& matrix);
            void SetColor(const colors::Color& color);
            void SetPasses(const Strings& passes);

        private:
            size_t CalculateStateHash() const override { YAGET_ASSERT(false, "Implemented CalculateStateHash methos!!!"); return 0; }
            void onUpdateGui(UpdateGuiType updateGuiType) override;

            void OnReset() override;
            void onRender(const RenderTarget* renderTarget, const math3d::Matrix& matrix) override;
            void OnRender(const RenderBuffer& /*renderBuffer*/, const DirectX::SimpleMath::Matrix& /*viewMatrix*/, const DirectX::SimpleMath::Matrix& /*projectionMatrix*/) override {}
            void OnGuiRender(const RenderBuffer& /*renderBuffer*/, const DirectX::SimpleMath::Matrix& /*viewMatrix*/, const DirectX::SimpleMath::Matrix& /*projectionMatrix*/) override {}

            struct PerObjectConstants
            {
                math3d::Matrix mMatrix;
                math3d::Color mColor = colors::White;
            };

            PerObjectConstants mPerObjectConstants;

            using DescriptionsWatcher = ResourceWatchCollection<render::DescriptionResource>;
            std::unique_ptr<DescriptionsWatcher> mDescriptionsWatcher;
            Strings mActivePasses;
        };

    } // namespace render
} // namespace yaget
