///////////////////////////////////////////////////////////////////////
// RenderTextures.h
//
//  Copyright 03/14/2026 Edgar Glowacki.
//
//  Maintained by: Edgar
//
//  NOTES:
//
//  #include "Render/Pipeline/RenderTextures.h"
//
//////////////////////////////////////////////////////////////////////
//! \file
#pragma once
#include "Render/RenderCore.h"
#include "Streams/Buffers.h"
#include "VTS/VirtualTransportSystem.h"
#include "Render/Cache/CacheWatcher.h"


namespace D3D12MA
{
    class Allocation;
}

struct ID3D12Resource;
struct ID3D12DescriptorHeap;

namespace yaget::render
{
    namespace commands
    {
        class RenderTarget;
    }

    class DeviceB;

    //-------------------------------------------------------------------------------------------------
    class RenderTextures : public CacheWatcher<io::Buffer>
    {
    public:
        RenderTextures(io::VirtualTransportSystem& vts, io::VirtualTransportSystem::Section fileName);
        ~RenderTextures();

        io::Buffer GetTexture(const io::Tag& tag);
        std::vector<io::Buffer> GetTextures(const io::Tags& tags, comp::gs::mt::InitCounter* counter);

        void Preload(const io::Tags& tags, comp::gs::mt::InitCounter& counter);

        static void PopulateMappings(io::VirtualTransportSystem::Section fileName, io::VirtualTransportSystem& vts);
        static void SaveMappings(io::VirtualTransportSystem::Section fileName, io::VirtualTransportSystem& vts);
    };


    //-------------------------------------------------------------------------------------------------
    class TextureResources
    {
    public:
        TextureResources(DeviceB& device, RenderTextures& renderTextures);
        ~TextureResources();

        ID3D12DescriptorHeap* GetResourceView(const io::Tag& tag) const;
        std::vector<ID3D12DescriptorHeap*> GetResourceViews(const io::Tags& tags) const;
        ID3D12Resource* GetResource(const io::Tag& tag);
        std::vector<ID3D12Resource*> GetResources(const io::Tags& tags, comp::gs::mt::InitCounter* counter);

        void AttachRenderTarget(const io::Tag& tag, const commands::RenderTarget* renderTarget);

        void Preload(const io::Tags& tags, comp::gs::mt::InitCounter& counter);

    private:
        DeviceB& mDevice;
        RenderTextures& mRenderTextures;

        struct ResourceData
        {
            unique_obj<D3D12MA::Allocation> mAllocation;
            ComPtr<ID3D12Resource> mResource;
            ComPtr<ID3D12DescriptorHeap> mDescriptorHeap;
        };

        using ResourceMap = std::map<io::Tag, ResourceData>;
        ResourceMap mResources;

        mutable std::shared_mutex mSharedMutex;
    };
}
