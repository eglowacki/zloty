/////////////////////////////////////////////////////////////////////////
// Device.h
//
//  Copyright June 19 2021 Edgar Glowacki.
//
// NOTES:
//      Another attempt at dx12 rendering
//
//      Create Window
//      Create Device
//      Create Command Queue
//      Create Swap Chain
//      Create Back Buffers
//      Create Root Signature
//      Create Pipeline State Object
//      Create Command Lists
//      Create Vertex Buffers and etc.
//      Loop
//          Populate Command Lists.
//          Execute Command Lists.
//          Wait(fence)
//          Display a beautifully rendered frame.
//          Reset the Command Listsand Allocators.
//      Begin clean up by using Wait(fence).
//      Release all D3D12 objects.
//      https://vzout.com/c++/directx12_tutorial.html
//
// #include "Render/Device.h"
//
/////////////////////////////////////////////////////////////////////////
//! \file

#pragma once

#include "YagetCore.h"
#include "Platform/Support.h"
#include <wrl/client.h>

#include <d3dx12.h>

namespace yaget
{
    namespace metrics { class Channel; }
    namespace time { class GameClock; }

    class Application;
}


namespace yaget::render
{
    namespace platform
    {
        class Adapter;
        class CommandQueue;
        class Fence;
        class SwapChain;
    }

    class Device : public Noncopyable<Device>
    {
    public:
        template <typename T>
        using ComPtr = Microsoft::WRL::ComPtr<T>;

        Device(Application& app);
        ~Device();

        void Resize();
        void SurfaceStateChange();

        void RenderFrame(const time::GameClock& gameClock, metrics::Channel& channel);

    private:
        static const UINT FrameCount = 2;

        struct Waiter
        {
            void Wait();

            void BeginPause();
            void EndPause();

            std::mutex mPauseRenderMutex;
            std::condition_variable mWaitForRenderThread;
            std::atomic_bool mPauseCounter{ false };
            std::condition_variable mRenderPaused;

            int mUsageCounter = 0;
        };

        struct WaiterScoper
        {
            WaiterScoper(Waiter& waiter) : waiter(waiter)
            {
                waiter.BeginPause();
            }

            ~WaiterScoper()
            {
                waiter.EndPause();
            }

            Waiter& waiter;
        };

        Application& mApplication;
        Waiter mWaiter;

        std::unique_ptr<platform::Adapter> mAdapter;
        std::unique_ptr<platform::CommandQueue> mCommandQueue;
        std::unique_ptr<platform::Fence> mFence;
        std::unique_ptr<platform::SwapChain> mSwapChain;

        // add-hock group
        uint32_t mFrameIndex;
        ComPtr<ID3D12DescriptorHeap> mRTVHeap;
        uint32_t mRTVDescriptorSize;
        ComPtr<ID3D12Resource> mRenderTargets[FrameCount];
        ComPtr<ID3D12CommandAllocator> mCommandAllocator;

        ComPtr<ID3D12RootSignature> mRootSignature;
        ComPtr<ID3D12PipelineState> mPipelineState;
        ComPtr<ID3D12GraphicsCommandList> mCommandList;

        ComPtr<ID3D12Resource> mVertexBuffer;
        D3D12_VERTEX_BUFFER_VIEW mVertexBufferView;

        CD3DX12_VIEWPORT mViewport;
        CD3DX12_RECT mScissorRect;
    };
}
