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

#include "Render/Commands/RenderCommandList.h"
#include "App/WindowFrame.h"
#include "Render/Waiter.h"

struct ID3D12GraphicsCommandList;
namespace { struct Framer; }

namespace yaget
{
    namespace metrics { class Channel; }
    namespace time { class GameClock; }
}

namespace yaget::render
{
    namespace commands
    {
        class CommandListStorage;
        class AllocatorStorage;
        class QueueStorage;
    }

    namespace platform
    {
        class Adapter;
        class SwapChain;
    }
    namespace info { struct Adapter; }

    //-------------------------------------------------------------------------------------------------
    class DeviceB : public NoCopy
    {
    public:
        DeviceB(app::WindowFrame windowFrame, const yaget::render::info::Adapter& adapterInfo);
        ~DeviceB();

        void Resize();
        void SurfaceStateChange();
        int64_t OnHandleRawInput(app::DisplaySurface::PlatformWindowHandle hWnd, uint32_t message, uint64_t wParam, int64_t lParam);

        void Shutdown();
        const platform::Adapter& GetAdapter() const { return *mAdapter.get(); }

        //--------------------------------
        // Some refactor for DX12 command classes
        struct FrameCommands
        {
            FrameCommands(DeviceB& device, const time::GameClock& gameClock, metrics::Channel& channel);
            ~FrameCommands();

            // This returns first CommandList.
            commands::CommandList* BeginFrame(const colors::Color* color);
            void EndFrame();

            // This will return next available CommandList
            commands::CommandList* GetAvailableCommandList(commands::Type commandType);

        private:
            commands::QueueStorage& GetQueueStorage() const;
            commands::AllocatorStorage& GetAllocatorStorage() const;
            commands::CommandListStorage& GetCommandListStorage() const;

            // Return next available CommandList and add it to mCommandsToRender
            commands::CommandListStorage::CommandListHandle FindNextFreeCommandList(commands::Type commandType);

            DeviceB* mDevice{};
            const time::GameClock* mGameClock{};
            metrics::Channel* mChannel{};
            std::vector<commands::CommandListStorage::CommandListHandle> mCommandsToRender;
        };

        FrameCommands GetFrameCommands(const time::GameClock& gameClock, metrics::Channel& channel);

    private:
        app::WindowFrame mWindowFrame;
        int mNumBackBuffers{};
        Waiter mWaiter;

        std::unique_ptr<platform::Adapter> mAdapter;
        std::unique_ptr<commands::QueueStorage> mQueueStorage;
        std::unique_ptr<commands::AllocatorStorage> mAllocatorStorage;
        std::unique_ptr<commands::CommandListStorage> mCommandListStorage;
        std::unique_ptr<platform::SwapChain> mSwapChain;

        // map of mapping between frame buffer index and which fence value is associated with that buffer index;
        using FrameFenceValues = std::map<uint32_t, uint64_t>;
        FrameFenceValues mFrameFenceValues;
    };

    // add class of type DeviceB but stub out all calls as a no-op
    class NullDevice : public NoCopy
    {
    public:
        NullDevice(app::WindowFrame /*windowFrame*/, const yaget::render::info::Adapter& /*adapterInfo*/) {}

        void Resize() {}
        void SurfaceStateChange() {}
        int64_t OnHandleRawInput(app::DisplaySurface::PlatformWindowHandle /*hWnd*/, uint32_t /*message*/, uint64_t /*wParam*/, int64_t /*lParam*/) { return 0; }
        void Shutdown();
    };
}
