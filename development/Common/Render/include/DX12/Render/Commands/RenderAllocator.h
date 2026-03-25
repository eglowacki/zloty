/////////////////////////////////////////////////////////////////////////
// RenderAllocator.h
//
//  Copyright 03/24/2026 Edgar Glowacki.
//
// NOTES:
//      This handles command allocators
//
// #include "Render/Commands/RenderAllocator.h"
//
/////////////////////////////////////////////////////////////////////////
//! \file

#pragma once

#include "Render/RenderCore.h"
#include "Render/Commands/RenderCommandTypes.h"

struct ID3D12Device;
struct ID3D12CommandAllocator;


namespace yaget::render::commands
{
    
    //-------------------------------------------------------------------------------------------------
    // == Not thread safe ==
    // A given allocator can be associated with no more than one currently recording command list at a time, 
    // though one command allocator can be used to create any number of GraphicsCommandList objects.
    // To reclaim the memory allocated by a command allocator, an app calls ID3D12CommandAllocator::Reset. 
    // This allows the allocator to be reused for new commands. App must make sure that the GPU is no longer 
    // executing any command lists which are associated with the allocator.
    //
    // Command list allocators can only be reset when the associated command lists have finished execution on the GPU. 
    // Apps should use fences to determine GPU execution progress.
    // We should create number of allocators based on number of back buffers
    class Allocator
    {
    public:
        Allocator(ID3D12Device* device, Type commandType);
        ~Allocator();

        ID3D12CommandAllocator* GetDeviceCommandAllocator() const;
        void Reset();

    private:
        ComPtr<ID3D12CommandAllocator> mCommandAllocator;
    };


    //-------------------------------------------------------------------------------------------------
    // We create numBuffers Allocators per Type
    class AllocatorStorage
    {
    public:
        AllocatorStorage(ID3D12Device* device, uint32_t numBuffers);
        ~AllocatorStorage();

        void Reset();
        Allocator* GetAllocator(Type commandType, uint32_t frameIndex);

    private:
        struct AllocatorEntry
        {
            std::unique_ptr<Allocator> mAllocator;
            uint32_t mBufferIndex{};
        };

        using StoredAllocators = std::vector<AllocatorEntry>;
        using Allocators = std::map<Type, StoredAllocators>;
        Allocators mAllocators;
    };

}
