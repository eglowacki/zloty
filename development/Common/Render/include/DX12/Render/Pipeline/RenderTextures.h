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

namespace yaget::render
{
    class DeviceB;

    //namespace platform
    //{
    //    class Adapter;
    //}


    //-------------------------------------------------------------------------------------------------
    class RenderTextures : public CacheWatcher<io::Buffer>
    {
    public:
        RenderTextures(io::VirtualTransportSystem& vts, io::VirtualTransportSystem::Section fileName);
        ~RenderTextures();

        io::Buffer GetTexture(const io::Tag& tag);
        std::vector<io::Buffer> GetTextures(const io::Tags& tags);

        static void PopulateMappings(io::VirtualTransportSystem::Section fileName, io::VirtualTransportSystem& vts);
        static void SaveMappings(io::VirtualTransportSystem::Section fileName, io::VirtualTransportSystem& vts);
    };


    //-------------------------------------------------------------------------------------------------
    class TextureResources
    {
    public:
        TextureResources(DeviceB& device, RenderTextures& renderTextures);
        ~TextureResources();

        ComPtr<ID3D12Resource> GetResource(const io::Tag& tag);
        std::vector<ComPtr<ID3D12Resource>> GetResources(const io::Tags& tags);

    private:
        DeviceB& mDevice;
        //const platform::Adapter& mAdapter;
        RenderTextures& mRenderTextures;

        struct ResourceData
        {
            ComPtr<ID3D12Resource> mResource;
            unique_obj<D3D12MA::Allocation> mAllocation;
        };

        using ResourceMap = std::map<io::Tag, ResourceData>;
        ResourceMap mResources;

        std::shared_mutex mSharedMutex;
    };
}
