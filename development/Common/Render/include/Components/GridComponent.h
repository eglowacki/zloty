/////////////////////////////////////////////////////////////////////////
// GridComponent.h
//
//  Copyright 7/26/2016 Edgar Glowacki.
//
// NOTES:
//      Render grid component
//
//
// #include "Components/GridComponent.h"
//
/////////////////////////////////////////////////////////////////////////
//! \file
#pragma once

#include "RenderComponent.h"
#include "PrimitiveBatch.h"
#include "VertexTypes.h"
#include "Effects.h"
#include "CommonStates.h"
#include <vector>
#include <memory>
#include <wrl/client.h>

namespace yaget
{
    namespace render
    {
        class Device;

        class GridComponent : public RenderComponent
        {
        public:
            GridComponent(comp::Id_t id, Device& device);
            virtual ~GridComponent();

        private:
            size_t CalculateStateHash() const override { YAGET_ASSERT(false, "Implemented CalculateStateHash methos!!!"); return 0; }

            void OnReset() override;
            void OnRender(const RenderBuffer& renderBuffer, const DirectX::SimpleMath::Matrix& viewMatrix, const DirectX::SimpleMath::Matrix& projectionMatrix) override;
            void OnGuiRender(const RenderBuffer& renderBuffer, const DirectX::SimpleMath::Matrix& viewMatrix, const DirectX::SimpleMath::Matrix& projectionMatrix) override;

            std::unique_ptr<DirectX::BasicEffect> mEffect;
            std::unique_ptr<DirectX::PrimitiveBatch<DirectX::VertexPositionColor>>  mBatch;
            std::unique_ptr<DirectX::CommonStates> mStates;
            Microsoft::WRL::ComPtr<ID3D11InputLayout> mInputLayout;
        };

    } // namespace render
} // namespace yaget
