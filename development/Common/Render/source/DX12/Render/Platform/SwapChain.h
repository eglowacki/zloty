/////////////////////////////////////////////////////////////////////////
// SwapChain.h
//
//  Copyright SwapChain.h Edgar Glowacki.
//
// NOTES:
//      
//
// #include "Render/Platform/SwapChain.h"
//
/////////////////////////////////////////////////////////////////////////
//! \file

#pragma once

#include "Render/RenderCore.h"
#include "App/WindowFrame.h"


struct ID3D12CommandAllocator;
struct ID3D12CommandQueue;
struct ID3D12DescriptorHeap;
struct ID3D12Device;
struct ID3D12GraphicsCommandList;
struct ID3D12Resource;
struct IDXGIFactory;
struct IDXGISwapChain4;

namespace yaget
{
    namespace app { class DisplaySurface; }
    namespace time { class GameClock; }
    namespace metrics { class Channel; }
    namespace render::info { struct Adapter; }
    namespace render { class Polygon; }
}


namespace yaget::render::platform
{
    //-------------------------------------------------------------------------------------------------
    class CommandQueue;

    class SwapChain
    {
    public:
        SwapChain(app::WindowFrame windowFrame, const yaget::render::info::Adapter& adapterInfo, ID3D12Device* device, IDXGIFactory* factory, ID3D12CommandQueue* commandQueue);
        ~SwapChain();

        void Resize();
        void Present(const time::GameClock& gameClock, metrics::Channel& channel);

        uint32_t GetCurrentBackBufferIndex() const { return mCurrentBackBufferIndex; }
        ID3D12Resource* GetCurrentRenderTarget() const;
        ID3D12DescriptorHeap* GetRTVDescriptorHeap() const;
        ID3D12DescriptorHeap* GetDSVDescriptorHeap() const;

        static int DepthBufferFlag;
        static int StencilBufferFlag;

    private:
        void UpdateRenderTargetViews();

        app::WindowFrame mWindowFrame;
        int mNumBackBuffers = 2;
        bool mTearingSupported = false;
        ID3D12Device* mDevice = nullptr;
        ComPtr<IDXGISwapChain4> mSwapChain;
        uint32_t mCurrentBackBufferIndex = 0;

        ComPtr<ID3D12DescriptorHeap> mRTVDescriptorHeap;
        std::vector<ComPtr<ID3D12Resource>> mBackBuffers;

        int mDepthStencilFlags = 0;

        ComPtr<ID3D12DescriptorHeap> mDSVDescriptorHeap;    // This is a heap for our depth/stencil buffer descriptor
        ComPtr<ID3D12Resource> mDepthStencilBuffer;         // This is the memory for our depth buffer. it will also be used for a stencil buffer in a later tutorial
    };

} // namespace yaget::render::platform

