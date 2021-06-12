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
#include <wrl/client.h>


struct ID3D12Device2;
struct ID3D12CommandQueue;
struct IDXGISwapChain4;
struct ID3D12DescriptorHeap;
struct ID3D12Resource;
struct ID3D12CommandAllocator;
struct ID3D12GraphicsCommandList;
struct ID3D12Fence;

namespace yaget { class Application; }

namespace yaget::render::platform
{
    class SwapChain
    {
    public:
        SwapChain(Application& app, ID3D12Device2* device, uint32_t numFrames);
        ~SwapChain();

        void Render();
        void Resize();

    private:
        using Handle_t = void*;

        void UpdateRenderTargetViews(ID3D12Device2* device);

        Application& mApplication;
        ID3D12Device2* mDevice = nullptr;
        Microsoft::WRL::ComPtr<ID3D12CommandQueue> mCommandQueue;
        Microsoft::WRL::ComPtr<IDXGISwapChain4> mSwapChain;
        Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> mDescriptorHeap;
        Microsoft::WRL::ComPtr<ID3D12Fence> mFence;
        Handle_t mFenceEvent;
        uint64_t mFenceValue = 0;
        uint32_t mDescriptorSize = 0;
        uint32_t mNumFrames = 2;

        using BackBuffer = Microsoft::WRL::ComPtr<ID3D12Resource>;
        std::vector<BackBuffer> mBackBuffers;

        using Allocator = Microsoft::WRL::ComPtr<ID3D12CommandAllocator>;
        std::vector<Allocator> mAllocators;

        std::vector<uint64_t> mFrameFenceValues;

        uint32_t mCurrentBackBufferIndex = 0;

        Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> mCommandList;
    };
}

