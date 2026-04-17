///////////////////////////////////////////////////////////////////////
// ShaderBuffers.h
//
//  Copyright 03/17/2026 Edgar Glowacki.
//
//  Maintained by: Edgar
//
//  NOTES:
//
//  #include "Render/Pipeline/ShaderBuffers.h"
//
//////////////////////////////////////////////////////////////////////
//! \file
#pragma once


#include "RenderShaders.h"
#include "VTS/VirtualTransportSystem.h"


namespace D3D12MA
{
    class Allocation;
}

namespace yaget::render
{
    class ConstantBuffer;

    namespace platform
    {
        class Adapter;
    }

    //--------------------------------------------------------------------------------------------------
    class ShaderBuffers
    {
    public:
        ShaderBuffers(int numBuffers, const platform::Adapter& adapter, io::VirtualTransportSystem& vts, io::VirtualTransportSystem::Section fileName);
        ~ShaderBuffers();

        void MakeBuffers(const io::Tag& tag, const RenderShaders::IndexMap& indexMap);
        ConstantBuffer* GetBuffer(const io::Tag& tag);

        ComPtr<ID3D12Resource> GetNextResource(uint32_t bufferIndex, size_t dataSize);

    private:
        void AddConstantResource(const io::Tag& tag, size_t size);

        std::shared_mutex mMutex;
        const platform::Adapter& mAdapter;

        using BuffersMap = std::map<io::Tag, std::shared_ptr<ConstantBuffer>>;
        BuffersMap mBuffersMap;

        uint32_t mCurrentIndexBuffer = std::numeric_limits<uint32_t>::max();

        struct ConstantResource
        {
            D3D12MA::Allocation* mAllocation{};
            ComPtr<ID3D12Resource> mResource;
            size_t mSize{};
            bool mUsed = false;
        };

        using ConstantResources = std::vector<ConstantResource>;
        std::map<int, ConstantResources> mConstantResources;

        // this is used to make sure that we have enough constant buffers for the number of ConstantResources.
        std::map<size_t, int> mNumberOfResources;
    };
}
