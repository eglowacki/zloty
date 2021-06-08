//////////////////////////////////////////////////////////////////////
// GeometryResource.h
//
//  Copyright 7/28/2018 Edgar Glowacki
//
//  Maintained by: Edgar
//
//  NOTES:
//      Geometry resource which encapsulates vertex and index data
//
//
//  #include "Resources/GeometryResource.h"
//
//////////////////////////////////////////////////////////////////////
//! \file
#pragma once

#include "YagetCore.h"
#include "Resources/ResourceView.h"
#include "ShaderResources.h"
#include "RenderMathFacade.h"

namespace yaget
{
    namespace io { namespace render {class GeomAsset; class PakAsset; }}

    namespace render
    {
        class GeometryResource : public ResourceView
        {
        public:
            GeometryResource(Device& device, std::shared_ptr<io::render::GeomAsset> asset);
            GeometryResource(Device& device, std::shared_ptr<io::render::PakAsset> asset);

            bool Activate() override;
            void UpdateGui(comp::Component::UpdateGuiType updateGuiType) override;
            const char* GetNameType() const override { return "Geometry"; }

            const math::Box& BoundingBox() const { return mBoundingBox; }

        private:

            struct VertexData
            {
                Microsoft::WRL::ComPtr<ID3D11Buffer> mIndexBuffer;
                Microsoft::WRL::ComPtr<ID3D11Buffer> mVertexbuffer;
                Microsoft::WRL::ComPtr<ID3D11Buffer> mUVbuffer;
                size_t mNumVerticies = 0;
                size_t mNumIndicies = 0;
                math::Box mBoundingBox;
            };

            math::Box mBoundingBox;

            std::vector<VertexData> mVertexData;

            Microsoft::WRL::ComPtr<ID3D11RasterizerState> mWireRasterizerState;
        };

    } // namespace render
} // namespace yaget