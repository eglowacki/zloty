/////////////////////////////////////////////////////////////////////////
// CommandListPool.h
//
//  Copyright 09/03/2022 Edgar Glowacki.
//
// NOTES:
//      Provides pool for mananaging and re-using command lists
//
// #include "Render/Platform/CommandListPool.h"
//
/////////////////////////////////////////////////////////////////////////
//! \file

#pragma once

#include "Render/RenderCore.h"
#include "Render/Platform/CommandQueue.h"

#include <queue>


struct ID3D12CommandAllocator;
struct ID3D12DescriptorHeap;
struct ID3D12Device;
struct ID3D12GraphicsCommandList4;
struct ID3D12Resource;

namespace colors { struct Color; } 

namespace yaget::render::platform
{
    //-------------------------------------------------------------------------------------------------
    class CommandListPool
    {
    public:
        CommandListPool(ID3D12Device* device, uint32_t numCommands);
        ~CommandListPool();

        struct Handle : private Noncopyable<Handle>
        {
            Handle(CommandListPool& commandPool, ComPtr<ID3D12GraphicsCommandList4> commandList, CommandQueue::Type type, ID3D12Resource* renderTarget, ID3D12DescriptorHeap* descriptorHeap, uint32_t frameIndex);
            ~Handle();

            void TransitionToRenderTarget();
            void TransitionToPresent(bool closeCommand);
            void ClearRenderTarget(const colors::Color& color);

            operator ID3D12GraphicsCommandList4*() const { return mCommandList.Get(); }
            ID3D12GraphicsCommandList4* operator->() const { return mCommandList.Get(); }

        private:
            CommandListPool& mCommandPool;
            ComPtr<ID3D12GraphicsCommandList4> mCommandList;
            CommandQueue::Type mType;

            ID3D12Resource* mRenderTarget = nullptr;
            ID3D12DescriptorHeap* mDescriptorHeap = nullptr;
            uint32_t mFrameIndex = 0;
        };

        Handle GetCommandList(CommandQueue::Type type, ID3D12CommandAllocator* commandAllocator, ID3D12Resource* renderTarget, ID3D12DescriptorHeap* descriptorHeap, uint32_t frameIndex);

        using CommandsList = std::queue<ComPtr<ID3D12GraphicsCommandList4>>;

    private:
        void FreeUsed(ComPtr<ID3D12GraphicsCommandList4> commandList, CommandQueue::Type type);

        std::map<CommandQueue::Type, CommandsList> mFreeCommandsList;
    };

} // namespace yaget::render::platform

