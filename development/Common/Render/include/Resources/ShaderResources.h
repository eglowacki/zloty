/////////////////////////////////////////////////////////////////////////
// ShaderResources.h
//
//  Copyright 3/5/2017 Edgar Glowacki.
//
// NOTES:
//      Wrapper for vertex and pixel shader
//
//
// #include "ShaderResources.h"
//
/////////////////////////////////////////////////////////////////////////
//! \file

#ifndef YAGET_SHADER_MATERIAL_H
#define YAGET_SHADER_MATERIAL_H
#pragma once

#include <d3d11_2.h>
#include "RenderMathFacade.h"
#include "RenderHelpers.h"
#include "Resources/ResourceView.h"
#include "Streams/Buffers.h"
#include "VTS/ResolvedAssets.h"
#include <wrl/client.h>

namespace yaget
{
    namespace io
    {
        namespace render
        {
            class ShaderAsset;
        }
    }

    namespace render
    {
        class Device;
        
        //--------------------------------------------------------------------------------------------------
        class VertexShaderResource : public ResourceView
        {
        public:
            VertexShaderResource(Device& device, std::shared_ptr<io::render::ShaderAsset> asset);

            bool Activate() override;
            const char* GetNameType() const override { return "Vertex Shader"; }

        private:
            Microsoft::WRL::ComPtr<ID3D11VertexShader> mVertexShader;
        };

        //--------------------------------------------------------------------------------------------------
        class PixelShaderResource : public ResourceView
        {
        public:
            PixelShaderResource(Device& device, std::shared_ptr<io::render::ShaderAsset> asset);

            bool Activate() override;
            const char* GetNameType() const override { return "Pixel Shader"; }

        private:
            Microsoft::WRL::ComPtr<ID3D11PixelShader> mPixelShader;
        };

        //--------------------------------------------------------------------------------------------------
        class InputLayoutResource : public ResourceView
        {
        public:
            InputLayoutResource(Device& device, std::shared_ptr<io::render::ShaderAsset> asset);

            bool Activate() override;
            const char* GetNameType() const override { return "Input Layout"; }

        private:
            Microsoft::WRL::ComPtr<ID3D11InputLayout> mInputLayout;
        };

    } // namespace render
} // namespace yaget

#endif // YAGET_SHADER_MATERIAL_H

