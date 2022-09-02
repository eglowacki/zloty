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
#include "Render/Platform/Commander.h"


struct IDXGISwapChain4;
struct IDXGIFactory;
struct ID3D12Device;
struct ID3D12DescriptorHeap;
struct ID3D12Resource;
struct ID3D12CommandAllocator;
struct ID3D12GraphicsCommandList;

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
    class CommandQueue;

    class SwapChain
    {
    public:
        SwapChain(app::WindowFrame windowFrame, const yaget::render::info::Adapter& adapterInfo, ID3D12Device* device, IDXGIFactory* factory);
        ~SwapChain();

        void Resize();
        void Render(const std::vector<Polygon*>& polygons, const time::GameClock& gameClock, metrics::Channel& channel);

    private:
        void UpdateRenderTargetViews();
        void PrepareCommandList(const time::GameClock& gameClock, ID3D12GraphicsCommandList* commandList, bool clearRenderTarget);

        app::WindowFrame mWindowFrame;
        int mNumBackBuffers = 2;
        bool mTearingSupported = false;
        ID3D12Device* mDevice = nullptr;
        std::unique_ptr<platform::CommandQueue> mCommandQueue;
        ComPtr<IDXGISwapChain4> mSwapChain;
        uint32_t mCurrentBackBufferIndex = 0;

        ComPtr<ID3D12DescriptorHeap> mRTVDescriptorHeap;
        Commander mCommander;

        std::vector<ComPtr<ID3D12Resource>> mBackBuffers;
        std::vector<ComPtr<ID3D12CommandAllocator>> mCommandAllocators;
        std::vector<uint64_t> mFrameFenceValues;

        ComPtr<ID3D12GraphicsCommandList> mCommandList;
        float mCurrentColorT = 0.0f;
        float mColorTDirection = 1.0f;
    };

} // namespace yaget::render::platform

