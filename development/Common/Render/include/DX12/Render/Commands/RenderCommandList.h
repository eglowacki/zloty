/////////////////////////////////////////////////////////////////////////
// RenderCommandList.h
//
//  Copyright 03/24/2026 Edgar Glowacki.
//
// NOTES:
//      This handles graphics command list
//
// #include "Render/Commands/RenderCommandList.h"
//
/////////////////////////////////////////////////////////////////////////
//! \file

#pragma once


#include "Render/RenderCore.h"
#include "RenderCommandTypes.h"

struct ID3D12Device;
struct ID3D12GraphicsCommandList;


namespace yaget::render::commands
{
    class Allocator;

    //-------------------------------------------------------------------------------------------------
    // == Not thread safe ==
    // You can also re-use an existing command list by calling ID3D12GraphicsCommandList::Reset, 
    // which also leaves the command list in the recording state. 
    // Unlike ID3D12CommandAllocator::Reset, you can call Reset while the command list is still being executed. 
    // A typical pattern is to submit a command list and then immediately reset it to reuse the allocated memory 
    // for another command list. Command list can be reset at any time and must be before re-recording.
    // Note that only one command list associated with each command allocator may be in a recording state at one time.
    //
    // PopulateCommandList():
    //  commandAllocator->Reset()
    //  commandList->Reset(commandAllocator, pipelineState)
    //  ...
    //  commandList->DrawInstanced(3, 1, 0, 0);
    //  commandList->ResourceBarrier(...)   // Indicate that the back buffer will now be used to present.
    //  commandList->Close()
    class CommandList
    {
    public:
        CommandList(ID3D12Device* device, Type commandType);
        ~CommandList();

        ID3D12GraphicsCommandList* GetDeviceCommandList() const;

        void Close();
        void Reset(Allocator* allocator);
        Type GetType() const { return mCommandType; }


    private:
        Type mCommandType;
        ComPtr<ID3D12GraphicsCommandList> mCommandList;
    };


    //-------------------------------------------------------------------------------------------------
    // We create numBuffers * 'Some_Predefine_Value' CommandList per Type
    class CommandListStorage
    {
    public:
        CommandListStorage(ID3D12Device* device, uint32_t numBuffers);
        ~CommandListStorage();

        class CommandListHandle : public NoCopy
        {
        public:
            CommandListHandle();
            CommandListHandle(CommandList* commandList, CommandListStorage* storage);
            ~CommandListHandle();

            CommandListHandle(CommandListHandle&& other) noexcept;
            CommandListHandle& operator=(CommandListHandle&& other) noexcept;

            operator CommandList*() const { return mCommandList; }
            CommandList* operator->() const { return mCommandList; }

            CommandList* Get() const { return mCommandList; }

        private:
            CommandList* mCommandList;
            CommandListStorage* mStorage;
        };

        CommandListHandle GetCommandList(Type commandType, uint32_t frameIndex);

    private:
        void FreeCommandList(CommandList* commandList);

        struct CommandListEntry
        {
            std::unique_ptr<CommandList> mCommandList;
            bool mUsed = false;
            uint32_t mBufferIndex{};
        };

        using StoredCommandLists = std::vector<CommandListEntry>;
        using Commands = std::map<Type, StoredCommandLists>;
        Commands mCommandLists;
    };

}
