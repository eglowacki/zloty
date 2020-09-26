//////////////////////////////////////////////////////////////////////
// RenderTarget.h
//
//  Copyright 7/19/2019 Edgar Glowacki
//
//  Maintained by: Edgar
//
//  NOTES:
//      
//
//
//  #include "RenderTarget.h"
//
//////////////////////////////////////////////////////////////////////
//! \file
#pragma once

#include "YagetCore.h"
#include "MathFacade.h"
#include <wrl/client.h>
#include <atomic>
#include <queue>


//namespace math3d { struct Color; }

namespace yaget
{
    namespace render
    {
        class Device;

        //--------------------------------------------------------------------------------------------------------
        // beginning of a RenderTarget class  
        // Device has ownership over render targets
        class RenderTarget
        {
        public:
            // Render target dimensions (width, height)
            using Dimension = std::pair<uint32_t, uint32_t>;
            static const uint32_t kDefaultSize = static_cast<uint32_t>(-1);

            // create render target of size.
            // If width/height is RenderTarget::kDefaultSize, then treat this as back buffer (swap chain)
            // // otherwise create renderable target texture of size (width/height)
            RenderTarget(Device& device, uint32_t width, uint32_t height, const std::string& name);
            ~RenderTarget();

            class Activator
            {
            public:
                Activator(RenderTarget& renderTarget) : mRenderTarget(renderTarget)
                {
                    mRenderTarget.BeginRender();
                }

                ~Activator()
                {
                    mRenderTarget.EndRender();
                }

            private:
                RenderTarget& mRenderTarget;
            };

            // we may not need this anymore if we rework rasterizer as a resource
            enum class ERasterizerState { WIRE_MODE = 0x01 };

            void SetRasterizerState(uint32_t rasterizerState);

            void SetClearColor(const math3d::Color& clearColor) { mClearColor = clearColor; }
            float GetAspectRatio() const;

            // NOTE: abomination, come up with better handling of using this texture as a sampler into a shader
            void UseAsSampler();

            enum class ProcessorType { PreFrame, PostFrame };
            using ProcessorFunction = std::function<void()>;
            void AddProcessor(ProcessorType processorType, ProcessorFunction processorFunction);

            template <typename T = uint32_t>
            auto Size() const { return std::pair(static_cast<T>(mDimension.first), static_cast<T>(mDimension.second)); }

            // take screen shot on the next render loop
            // NOTE: we should try to encapsulate this behavior
            void TakeScreenshot(const char* fileName);
            // used by render loop to see if the screen shot needs to be taken
            bool IsScreenshotToTake() const;

        private:
            friend Device;

            void BeginRender();
            void EndRender();
            void FreeResources();
            void CreateResources(uint32_t width, uint32_t height);

            // if IsScreenshotToTake returns true, call this file to save it to disk and reset the flag
            void SaveToFile() const;

            void ProcessStage(ProcessorType processorType);

            Device& mDevice;
            bool mSwapChain{ false };
            Microsoft::WRL::ComPtr<ID3D11Texture2D> mTextureMap;
            Microsoft::WRL::ComPtr<ID3D11RenderTargetView> mViewMap;
            Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> mShaderViewMap;
            Microsoft::WRL::ComPtr<ID3D11SamplerState> mSamplerSate;

            Microsoft::WRL::ComPtr<ID3D11Texture2D> mDepthStencilBuffer;
            Microsoft::WRL::ComPtr<ID3D11DepthStencilView> mDepthStencilView;
            Microsoft::WRL::ComPtr<ID3D11DepthStencilState> mDepthStencilState;

            Dimension mDimension;
            std::string mName;

            bool mVSync{ true };
            math3d::Color mClearColor{ 0.0f, 0.2f, 0.4f, 1.0f };
            std::atomic_bool mTakeScreenshot{ false };
            std::string mScreenshotFileName;

            Microsoft::WRL::ComPtr<ID3D11RasterizerState> mDefaultRasterizerState;
            Microsoft::WRL::ComPtr<ID3D11RasterizerState> mWireRasterizerState;
            uint32_t mRasterizerState{ 0 };

            using Processors = std::queue<ProcessorFunction>;

            using ProcessorsStage = std::unordered_map<ProcessorType, Processors>;
            ProcessorsStage mProcessorsStage;
        };

    } // namespace render
} // namespace yaget
