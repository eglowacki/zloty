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
//      https://www.3dgep.com/learning-directx-12-2/
// 
//      NVidia Do's & Dont's
//      https://developer.nvidia.com/dx12-dos-and-donts
//
// #include "Render/Device.h"
//
/////////////////////////////////////////////////////////////////////////
//! \file

#pragma once

#include "AdapterInfo.h"
#include "Render/Commands/RenderCommandList.h"
#include "App/WindowFrame.h"
#include "Render/Waiter.h"

struct ID3D12GraphicsCommandList;
struct ID3D12Resource;
struct ID3D12DescriptorHeap;

namespace
{
    struct Framer;
}

namespace yaget
{
    namespace metrics
    {
        class Channel;
    }

    namespace time
    {
        class GameClock;
    }
}

namespace yaget::render
{
    namespace commands
    {
        struct DepthStencilClear;
        class RenderTarget;
        class CommandListStorage;
        class AllocatorStorage;
        class QueueStorage;
    }

    namespace platform
    {
        class Adapter;
        class SwapChain;
    }

    namespace info
    {
        struct Adapter;
    }

    //-------------------------------------------------------------------------------------------------
    class DeviceB : public NoCopy
    {
    public:
        DeviceB(app::WindowFrame windowFrame, const info::Adapter& adapterInfo);
        ~DeviceB();

        void Resize();
        void SurfaceStateChange();
        int64_t OnHandleRawInput(app::DisplaySurface::PlatformWindowHandle hWnd, uint32_t message, uint64_t wParam, int64_t lParam);

        void Shutdown();
        const platform::Adapter& GetAdapter() const { return *mAdapter.get(); }
        platform::SwapChain& GetSwapChain() const;
        const info::Adapter& GetSelectedAdapter() const { return mSelectedAdapter; }
        const app::WindowFrame& GetWindowFrame() const;

        // this allows us to register for device resizing, so dependent resource
        // cna be reset and recreated.
        enum class ResizeState
        {
            Reset,
            Set
        };
        using ResizeCallback = std::function<void(const app::WindowFrame& windowFrame, ResizeState resizeState)>;

        size_t RegisterResizeCallback(ResizeCallback callback);
        void UnregisterResizeCallback(size_t callbackId);

        //--------------------------------
        // Some refactor for DX12 command classes
        struct FrameCommands
        {
            FrameCommands(DeviceB& device, const time::GameClock& gameClock, metrics::Channel& channel, commands::RenderTarget* selectedRenderTarget);
            FrameCommands(DeviceB& device);
            ~FrameCommands();

            // This returns first CommandList.
            commands::CommandList* BeginFrame(const math3d::Color* color, const commands::DepthStencilClear* clearDepthStencil);
            void EndFrame();
            uint32_t GetFrameIndex() const { return mFrameIndex; }

            // This will return next available CommandList
            commands::CommandList* GetAvailableCommandList(commands::Type commandType);

        private:
            enum class FrameType
            {
                Render,
                Copy
            };

            FrameCommands(DeviceB& device, const time::GameClock* gameClock, metrics::Channel* channel, FrameType frameType, commands::RenderTarget* selectedRenderTarget);
            static commands::Type GetCommandType(FrameType frameType);

            commands::QueueStorage& GetQueueStorage() const;
            commands::AllocatorStorage& GetAllocatorStorage() const;
            commands::CommandListStorage& GetCommandListStorage() const;

            // Return next available CommandList and add it to mCommandsToRender
            commands::CommandListStorage::CommandListHandle FindNextFreeCommandList(commands::Type commandType);

            DeviceB* mDevice{};
            uint32_t mFrameIndex{};
            const time::GameClock* mGameClock{};
            metrics::Channel* mChannel{};
            std::vector<commands::CommandListStorage::CommandListHandle> mCommandsToRender;

            FrameType mFrameType{};
            commands::RenderTarget* mSelectedRenderTarget{};
        };

        FrameCommands GetFrameCommands(commands::RenderTarget& selectedRenderTarget, const time::GameClock& gameClock, metrics::Channel& channel);
        FrameCommands GetCopyCommands();

    private:
        struct MemoryTrackerReporter
        {
            MemoryTrackerReporter() = default;
            ~MemoryTrackerReporter();
        };

        MemoryTrackerReporter mMemoryTrackerReporter;
        app::WindowFrame mWindowFrame;
        int mNumBackBuffers{};
        Waiter mWaiter;

        std::unique_ptr<platform::Adapter> mAdapter;
        std::unique_ptr<commands::QueueStorage> mQueueStorage;
        std::unique_ptr<commands::AllocatorStorage> mAllocatorStorage;
        std::unique_ptr<commands::CommandListStorage> mCommandListStorage;
        std::unique_ptr<platform::SwapChain> mSwapChain;

        // map of mapping between frame buffer index and which fence value is associated with that buffer index;
        using FenceValueArray = uint64_t[commands::Type::Max];
        using FrameFenceValues = std::map<uint32_t, FenceValueArray>;
        FrameFenceValues mFrameFenceValues;

        void SetFrameFenceValue(uint64_t fenceValue, uint32_t frameIndex, commands::Type type);
        uint64_t GetFrameFenceValue(uint32_t frameIndex, commands::Type type) const;
        uint64_t GetFrameFenceValue(commands::Type type) const;

        info::Adapter mSelectedAdapter;

        struct ResizeCallbackData
        {
            size_t mId;
            ResizeCallback mCallback;
        };

        using ResizeCallbacks = std::vector<ResizeCallbackData>;
        ResizeCallbacks mResizeCallbacks;
        size_t mNextResizeCallbackId{};
    };

    // add class of type DeviceB but stub out all calls as a no-op
    class NullDevice : public NoCopy
    {
    public:
        NullDevice(app::WindowFrame /*windowFrame*/, const info::Adapter& /*adapterInfo*/) {}
        void Resize() {}
        void SurfaceStateChange() {}
        int64_t OnHandleRawInput(app::DisplaySurface::PlatformWindowHandle /*hWnd*/, uint32_t /*message*/, uint64_t /*wParam*/, int64_t /*lParam*/) { return 0; }
        void Shutdown() {};
    };
}
