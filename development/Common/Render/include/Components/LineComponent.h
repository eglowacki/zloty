/////////////////////////////////////////////////////////////////////////
// LineComponent.h
//
//  Copyright 7/30/2016 Edgar Glowacki.
//
// NOTES:
//      Render lines
//
//
// #include "Components/LineComponent.h"
//
/////////////////////////////////////////////////////////////////////////
//! \file
#pragma once

#include "Components/RenderComponent.h"
#include "Components/PayloadStager.h"
#include "MathFacade.h"
#include "PrimitiveBatch.h"
#include "Debugging/Primitives.h"
#include <wrl/client.h>

namespace DirectX {
    struct VertexPositionColor;
    class CommonStates;
    class BasicEffect;
}

namespace yaget
{
    namespace render
    {
        class Device;

        class LineComponent : public RenderComponent
        {
        public:
            LineComponent(comp::Id_t id, Device& device, bool bScreenSpace);
            virtual ~LineComponent();

            using Line = render::primitives::Line;
            using Lines = render::primitives::Lines;

            using Lines_t = render::primitives::Lines;
            using LinesPtr_t = render::primitives::LinesPtr;

            using Stager = comp::PayloadStager<Lines>;
            using LinesPayload = Stager::Payload;
            using ConstLinesPayload = Stager::ConstPayload;

            LinesPayload GetNewPayloadLines() { return mLineStager.CreatePayload(); }

            //void Draw(LinesPtr_t lines);
            // NOTE: change this to PostToRender(...)
            void Draw(const LinesPayload& lines);

        private:
            size_t CalculateStateHash() const override { YAGET_ASSERT(false, "Implemented CalculateStateHash methos!!!"); return 0; }

            void OnReset() override;
            void onRender(const RenderTarget* renderTarget, const math3d::Matrix& matrix) override;
            void OnRender(const RenderBuffer& renderBuffer, const DirectX::SimpleMath::Matrix& viewMatrix, const DirectX::SimpleMath::Matrix& projectionMatrix) override;
            void OnGuiRender(const RenderBuffer& renderBuffer, const DirectX::SimpleMath::Matrix& viewMatrix, const DirectX::SimpleMath::Matrix& projectionMatrix) override;

            LinesPtr_t mLinesToRender;
            std::unique_ptr<DirectX::BasicEffect> mEffect;
            std::unique_ptr<DirectX::PrimitiveBatch<DirectX::VertexPositionColor>>  mBatch;
            std::unique_ptr<DirectX::CommonStates> mStates;
            Microsoft::WRL::ComPtr<ID3D11InputLayout> mInputLayout;

            bool mScreenSpace = false;

            Stager mLineStager;
        };

    } // namespace render
} // namespace yaget
