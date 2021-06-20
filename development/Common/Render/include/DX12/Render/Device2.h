//////////////////////////////////////////////////////////////////////
// Device.h
//
//  Copyright 06/06/2021 Edgar Glowacki
//
//  Maintained by: Edgar
//
//  NOTES:
//      We will structure our application like this:
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
//
//  #include "Render/Device.h"
//
//////////////////////////////////////////////////////////////////////
//! \file
#pragma once

#include "YagetCore.h"
#include "MathFacade.h"
#include <d3d12.h>


#include <wrl/client.h>


struct ID3D12Device;
struct ID3D12CommandQueue;
struct ID3D12CommandAllocator;
struct ID3D12Fence;
struct ID3D12Resource;
struct ID3D12DescriptorHeap;
struct IDXGISwapChain3;
struct ID3D12RootSignature;
struct ID3D12PipelineState;
struct ID3D12GraphicsCommandList;
struct ID3D12DebugDevice;

namespace yaget
{
    namespace metrics { class Channel; }
    namespace time { class GameClock; }

    class Application;
}

namespace yaget::render
{
    namespace platform { class HardwareDevice; class DeviceDebugger; class Adapter; }

    class Device : public Noncopyable<Device>
    {
    public:
        // Uniform data
        struct UniformData
        {
            math3d::Matrix mProjectionMatrix;
            math3d::Matrix mModelMatrix;
            math3d::Matrix mViewMatrix;
        };

        struct Vertex
        {
            float position[3];
            float color[3];
        };

        Device(Application& app);
        ~Device();

        void Resize();
        void SurfaceStateChange();

        void RenderFrame(const time::GameClock& gameClock, metrics::Channel& channel);

    private:
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

        void ResetBackBuffers();
        void SetupSwapChain();
        void InitBackBuffers();

        void InitializeResources();

        Application& mApplication;

#if YAGET_DEBUG_RENDER == 1
        std::unique_ptr<platform::DeviceDebugger> mDeviceDebugger;
#endif
        std::unique_ptr<platform::Adapter> mAdapter;
        Microsoft::WRL::ComPtr<ID3D12Device> mDevice;
#if YAGET_DEBUG_RENDER == 1
        Microsoft::WRL::ComPtr<ID3D12DebugDevice> mDebugDevice;
#endif
        Microsoft::WRL::ComPtr<ID3D12CommandQueue> mCommandQueue;
        Microsoft::WRL::ComPtr<ID3D12CommandAllocator> mCommandAllocator;
        Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> mCommandList;

        Microsoft::WRL::ComPtr<ID3D12Fence> mFence;
        uint64_t mFenceValue = 0;
        HANDLE mFenceEvent = nullptr;

        using BackBuffer = Microsoft::WRL::ComPtr<ID3D12Resource>;
        std::vector<BackBuffer> mBackBuffers;

        Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> mRtvHeap;

        Microsoft::WRL::ComPtr<IDXGISwapChain3> mSwapchain;
        uint32_t mFrameIndex = 0;
        uint32_t mRtvDescriptorSize = 0;
        Microsoft::WRL::ComPtr<ID3D12RootSignature> mRootSignature;
        Microsoft::WRL::ComPtr<ID3D12PipelineState> mPipelineState;                                   

        Microsoft::WRL::ComPtr<ID3D12Resource> mUniformBuffer;
        Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> mUniformBufferHeap;
        uint8_t* mMappedUniformBuffer = nullptr;

        UniformData mUboVs;

        Microsoft::WRL::ComPtr<ID3D12Resource> mVertexBuffer;
        Microsoft::WRL::ComPtr<ID3D12Resource> mIndexBuffer;

        D3D12_VERTEX_BUFFER_VIEW mVertexBufferView;
        D3D12_INDEX_BUFFER_VIEW mIndexBufferView;

        Vertex mVertexBufferData[3] = { {{ 1.0f, -1.0f, 0.0f}, {1.0f, 0.0f, 0.0f}},
                                        {{-1.0f, -1.0f, 0.0f}, {0.0f, 1.0f, 0.0f}},
                                        {{ 0.0f,  1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}} };

        uint32_t mIndexBufferData[3] = { 0, 1, 2 };

        //std::unique_ptr<platform::HardwareDevice> mHardwareDevice;
        Waiter mWaiter;
    };

}
