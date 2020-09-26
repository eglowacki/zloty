//////////////////////////////////////////////////////////////////////
// RenderTargetResource.h
//
//  Copyright 8/12/2019 Edgar Glowacki
//
//  Maintained by: Edgar
//
//  NOTES:
//      
//
//
//  #include "Resources/RenderTargetResource.h"
//
//////////////////////////////////////////////////////////////////////
//! \file
#pragma once

#include "YagetCore.h"
#include "Resources/ResourceView.h"
#include <wrl/client.h>


namespace yaget
{
    namespace io::render { class RenderTargetAsset; }

    namespace render
    {

        // to copy between resources use: ID3D11DeviceContext::CopyResource
        class RenderTargetResource : public ResourceView
        {
        public:
            RenderTargetResource(Device& device, std::shared_ptr<io::render::RenderTargetAsset> asset);
            ~RenderTargetResource();

            bool Activate() override;
            const char* GetNameType() const override { return "Render Target"; }

        private:
            using Dimension = std::pair<uint32_t, uint32_t>;

            Dimension mDimension;
            math3d::Color mClearColor;
            std::string mName;

            Microsoft::WRL::ComPtr<ID3D11Texture2D> mTextureMap;
            Microsoft::WRL::ComPtr<ID3D11RenderTargetView> mViewMap;
            //Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> mShaderViewMap;
            //Microsoft::WRL::ComPtr<ID3D11SamplerState> mSamplerSate;
            Microsoft::WRL::ComPtr<ID3D11Texture2D> mDepthStencilBuffer;
            Microsoft::WRL::ComPtr<ID3D11DepthStencilView> mDepthStencilView;
            Microsoft::WRL::ComPtr<ID3D11DepthStencilState> mDepthStencilState;
        };

    } // namespace render

} // namespace yaget
