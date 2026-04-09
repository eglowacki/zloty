///////////////////////////////////////////////////////////////////////
// RenderTarget.h
//
//  Copyright 03/30/2026 Edgar Glowacki.
//
//  Maintained by: Edgar
//
//  NOTES:
//      
//
//  #include "Render/Commands/RenderTarget.h"
//
//////////////////////////////////////////////////////////////////////
//! \file
#pragma once

#include "MathFacade.h"
#include "Render/RenderCore.h"
#include "Streams/Buffers.h"
#include <d3dx12.h>
#include <shared_mutex>

namespace yaget::app
{
    class WindowFrame;
}

namespace yaget::time
{
    class GameClock;
}

namespace yaget::metrics
{
    class Channel;
}

namespace yaget::render::platform
{
    class SwapChain;
}


namespace yaget::render::commands
{
    class CommandList;

    //-------------------------------------------------------------------------------------------------
    class RenderTarget
    {
    public:
        RenderTarget(ID3D12Device* device, int sizeX, int sizeY, DXGI_FORMAT renderTargetFormat, DXGI_FORMAT depthStencilFormat, ID3D12DescriptorHeap* depthStencilDescriptorHeap, ID3D12Resource* depthStencilResource);
        RenderTarget(platform::SwapChain& swapChain);
        ~RenderTarget();

        ID3D12Resource* Resource() const { return mRenderTargetResource.Get(); }
        ID3D12DescriptorHeap* SRVDescriptorHeap() const { return mSRVDescriptorHeap.Get(); }
        ID3D12DescriptorHeap* RTVDescriptorHeap() const { return mRTVDescriptorHeap.Get(); }

        void BeginFrame(const CommandList* commandList);
        void EndFrame(const CommandList* commandList);

        bool Present(const time::GameClock& /*gameClock*/, metrics::Channel& /*channel*/);

    private:
        // this is used in pixel shader as source texture
        ComPtr<ID3D12DescriptorHeap> mSRVDescriptorHeap{};
        // this is used as render target texture
        ComPtr<ID3D12DescriptorHeap> mRTVDescriptorHeap{};
        // depth stencil buffer
        ComPtr<ID3D12DescriptorHeap> mDSVDescriptorHeap{};

        DXGI_FORMAT mRenderTargetFormat;
        D3D12_RESOURCE_STATES mState;
        colors::Color mClearColor;
        ComPtr<ID3D12Resource> mRenderTargetResource;
        ComPtr<ID3D12Resource> mDepthStencilResource;
        platform::SwapChain* mSwapChain;
    };


    //-------------------------------------------------------------------------------------------------
    class RenderTargetStorage
    {
    public:
        RenderTargetStorage(ID3D12Device* device);
        ~RenderTargetStorage();

        // create (or return existing) render target based on passed parameters
        RenderTarget* GetRenderTarget(const io::Tag& tag, uint32_t sizeX, uint32_t sizeY, DXGI_FORMAT renderTargetFormat, DXGI_FORMAT depthStencilFormat, ID3D12DescriptorHeap* depthStencilDescriptorHeap, ID3D12Resource* depthStencilResource);
        // Wrap RenderTarget around swapChain
        RenderTarget* AliasRenderTarget(const io::Tag& tag, platform::SwapChain& swapChain);
        // If render target exists, return it, otherwise return nullptr
        RenderTarget* FindRenderTarget(const io::Tag& tag) const;

        void ResetAll(const app::WindowFrame& windowFrame);

    private:
        struct RenderTargetData
        {
            size_t mHash{};
            std::shared_ptr<RenderTarget> mRenderTarget;
        };
        using RenderTargetMap = std::map<io::Tag, RenderTargetData>;

        static RenderTarget* FindRenderTarget(const yaget::io::Tag& tag, size_t hashValue, const RenderTargetMap& renderTargetMap);


        mutable std::shared_mutex mMutex;
        RenderTargetMap mRenderTargetMap;
        ID3D12Device* mDevice;
    };


    //-------------------------------------------------------------------------------------------------
    // Return new RenderTarget based on swapChain size and format
    RenderTarget* CreateRenderTargetFrom(const io::Tag& tag, const platform::SwapChain& swapChain, RenderTargetStorage& renderTargetStorage);
    
}
