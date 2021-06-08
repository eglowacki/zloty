/////////////////////////////////////////////////////////////////////////
// ModelComponent.h
//
//  Copyright 7/26/2016 Edgar Glowacki.
//
// NOTES:
//     Houses any render specific data and logic
//
//
// #include "Components/ModelComponent.h"
//
/////////////////////////////////////////////////////////////////////////
//! \file
#pragma once

#include "Components/RenderComponent.h"
#include "RenderMathFacade.h"
#include "CommonStates.h"
#include "Resources/ShaderResources.h"
#include "ThreadModel/Variables.h"
#include "Resources/ResourceView.h"
#include "Resources/SkinShaderResource.h"
#include "Resources/GeometryResource.h"
#include <wrl/client.h>
#include <memory>
#include <vector>

namespace yaget
{
    namespace io
    {
        namespace render
        {
            class GeomAsset;
        }
    }

    namespace render
    {
        class Device;
        class TextureResource;
        class ModelComponentPool;
        struct AssetDescription;

        class ModelComponent : public RenderComponent
        {
        public:
            static const uint32_t SignalViewCreated = "ModelComponent.SignalViewCreated"_crc32;

            using Filler_t = std::function<void(const GeometryConvertor::GeometryData& verticies)>;

            ModelComponent(comp::Id_t id, Device& device);
            virtual ~ModelComponent()
            {}

            void SetColor(const math3d::Color& color) { mColor = color; }
            math::Box BoundingBox() const;

            //--------------------------------------------------------------------------------------------------
            struct ShaderData
            {
                math3d::Matrix mMatrix;
                math3d::Color mColor = DirectX::Colors::White;
            };

            struct VertexData
            {
                Microsoft::WRL::ComPtr<ID3D11Buffer> mIndexBuffer;
                Microsoft::WRL::ComPtr<ID3D11Buffer> mVertexbuffer;
                Microsoft::WRL::ComPtr<ID3D11Buffer> mUVbuffer;
                size_t mNumVerticies = 0;
                size_t mNumIndicies = 0;
                math::Box mBoundingBox;
                math3d::Matrix mMatrix;
            };

        private:
            size_t CalculateStateHash() const override { YAGET_ASSERT(false, "Implemented CalculateStateHash methos!!!"); return 0; }

            void OnReset() override;
            void OnRender(const RenderBuffer& renderBuffer, const math3d::Matrix& viewMatrix, const math3d::Matrix& projectionMatrix) override;
            void OnGuiRender(const RenderBuffer& renderBuffer, const DirectX::SimpleMath::Matrix& viewMatrix, const DirectX::SimpleMath::Matrix& projectionMatrix) override;

            math3d::Color mColor = DirectX::Colors::White;
            std::vector<VertexData> mVertexData;
            std::mutex mVertexDataMutex;

            math3d::Matrix mMatrix = math3d::Matrix::Identity;
            friend ModelComponentPool;

            mt::SmartVariable<GeometryResource> mGeometry;
            mt::SmartVariable<SkinShaderResource> mSkinShader;
        };

        class QuadComponent : public RenderComponent
        {
        public:
            QuadComponent(comp::Id_t id, Device& device);
            virtual ~QuadComponent()
            {}

            void SetScreenMatrix(const math3d::Matrix& matrix) { mScreenMatrix = matrix; }

        private:
            size_t CalculateStateHash() const override { YAGET_ASSERT(false, "Implemented CalculateStateHash methos!!!"); return 0; }

            void OnReset() override;
            void OnRender(const RenderBuffer& renderBuffer, const math3d::Matrix& viewMatrix, const math3d::Matrix& projectionMatrix) override;
            void OnGuiRender(const RenderBuffer& renderBuffer, const DirectX::SimpleMath::Matrix& viewMatrix, const DirectX::SimpleMath::Matrix& projectionMatrix) override;

            math3d::Matrix mScreenMatrix;
            Microsoft::WRL::ComPtr<ID3D11Buffer> mModelConstants;
            Microsoft::WRL::ComPtr<ID3D11InputLayout> mInputLayout;
            Microsoft::WRL::ComPtr<ID3D11RasterizerState> mRasterizerState;
            math3d::Color mColor = DirectX::Colors::White;

            struct VertexData
            {
                Microsoft::WRL::ComPtr<ID3D11Buffer> mIndexBuffer;
                Microsoft::WRL::ComPtr<ID3D11Buffer> mVertexbuffer;
                size_t mNumVerticies = 0;
                size_t mNumIndicies = 0;
            };
            std::vector<VertexData> mVertexData;

            struct ShaderData
            {
                math3d::Matrix mMatrix;
                math3d::Color mColor = DirectX::Colors::White;
            };
            math3d::Matrix mMatrix = math3d::Matrix::Identity;
            std::shared_ptr<TextureResource> mTextureResource;
        };

    } // namespace render
} // namespace yaget
