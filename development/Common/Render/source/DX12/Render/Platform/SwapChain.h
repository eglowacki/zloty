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


#include "YagetCore.h"
#include "App/WindowFrame.h"
#include "Render/RenderCore.h"


struct IDXGISwapChain4;
struct IDXGIFactory4;
struct ID3D12Device4;
struct ID3D12CommandQueue;
struct ID3D12DescriptorHeap;
struct ID3D12Resource;
struct ID3D12CommandAllocator;
struct ID3D12GraphicsCommandList2;

namespace yaget
{
    namespace app { class DisplaySurface; }
    namespace time { class GameClock; }
    namespace metrics { class Channel; }
}


namespace yaget::render::platform
{
    class CommandQueue;

    class SwapChain
    {
    public:
        SwapChain(app::WindowFrame windowFrame, const ComPtr<ID3D12Device4>& device, IDXGIFactory4* factory);
        ~SwapChain();

        void Resize();
        void Render(const time::GameClock& gameClock, metrics::Channel& channel);

    private:
        void UpdateRenderTargetViews();

        app::WindowFrame mWindowFrame;
        int mNumBackBuffers = 2;
        bool mTearingSupported = false;
        ComPtr<ID3D12Device4> mDevice;
        std::unique_ptr<platform::CommandQueue> mCommandQueue;
        ComPtr<IDXGISwapChain4> mSwapChain;
        uint32_t mCurrentBackBufferIndex = 0;

        ComPtr<ID3D12DescriptorHeap> mRTVDescriptorHeap;
        uint32_t mRTVDescriptorSize = 0;

        std::vector<ComPtr<ID3D12Resource>> mBackBuffers;
        std::vector<ComPtr<ID3D12CommandAllocator>> mCommandAllocators;
        std::vector<uint64_t> mFrameFenceValues;

        ComPtr<ID3D12GraphicsCommandList2> mCommandList;
        float mCurrentColorT = 0.0f;
        float mColorTDirection = 1.0f;
    };

} // namespace yaget::render::platform

