///////////////////////////////////////////////////////////////////////
// RenderGeometries.h
//
//  Copyright 03/29/2026 Edgar Glowacki.
//
//  Maintained by: Edgar
//
//  NOTES:
//
//  #include "Render/Pipeline/RenderGeometries.h"
//
//////////////////////////////////////////////////////////////////////
//! \file
#pragma once

#include "Render/RenderCore.h"
#include "Streams/Buffers.h"
#include "Render/Cache/CacheWatcher.h"

struct ID3D12Device;
struct ID3D12Resource;

namespace D3D12MA
{
    class Allocator;
    class Allocation;
}

namespace yaget::render
{
    class DeviceB;

    namespace geom
    {
        // format of binary geometry file
        // messagingBuffer.WriteDataChunk(&YagetFileSignature, sizeof(YagetFileSignature));
        // messagingBuffer.WriteDataChunk(&Header, sizeof(Header));
           
        // messagingBuffer.WriteDataChunk(static_cast<uint8_t*>(vertices.data()), vertices.size() * sizeof(V::value_type));
        // messagingBuffer.WriteDataChunk(static_cast<uint8_t*>(indices.data()), indices.size() * sizeof(I::value_type));

        struct Header
        {
            AssetCacheType mVertexFormat = AssetCacheType::Empty;
            size_t mVertexFormatSize{}; // size of the vertex structure in bytes
            size_t mIndexFormatSize{};  // size of the index element in bytes
            size_t mNumVertices{};      // number of total vertices in the vertex buffer
            size_t mNumIndices{};       // number of total indices in the index buffer. If this is 0, then the geometry is non-indexed
                                        // and number of triangles is mNumVertices/3, otherwise mNumIndices/3

            bool IsValid() const
            {
                return mVertexFormat != AssetCacheType::Empty && mNumVertices;
            }

            uint32_t VertexStride() const
            {
                return static_cast<uint32_t>(mVertexFormatSize);
            }

            uint32_t NumTriangles() const
            {
                if (mNumIndices)
                {
                    return static_cast<uint32_t>(mNumIndices / 3);
                }

                return static_cast<uint32_t>(mNumVertices / 3);
            }
            
            uint32_t VertexBufferSize() const
            {
                return static_cast<uint32_t>(mNumVertices * mVertexFormatSize);
            }
            
            uint32_t IndexBufferSize() const
            {
                return static_cast<uint32_t>(mNumIndices * mIndexFormatSize);
            }
        };

        constexpr size_t HeaderBufferSize = sizeof(YagetFileSignature) + sizeof(Header);

        bool ValidateDataLayout(const io::Buffer& buffer);
        inline const Header* GetHeader(const io::Buffer& buffer)
        {
            if (ValidateDataLayout(buffer))
            {
                return reinterpret_cast<const Header*>(io::cast_data<const char>(buffer) + sizeof(YagetFileSignature));
            }

            return nullptr;
        }

        // To use this class
        // geom::DataLayout<DirectX::VertexPositionColorTexture> dataLayout(buffer);
        template<typename V, typename I = uint32_t>
        struct DataLayout
        {
            using VertexType = V;
            using IndexType = I;

            DataLayout(const io::Buffer& buffer)
                : mHeader{ reinterpret_cast<const Header*>(io::cast_data<const char>(buffer) + sizeof(YagetFileSignature)) }
                , mVertices{ reinterpret_cast<const V*>(io::cast_data<const char>(buffer) + HeaderBufferSize) }
                , mIndices{ reinterpret_cast<const I*>(io::cast_data<const char>(buffer) + HeaderBufferSize + sizeof(V) * mHeader->mNumVertices) }
            {
                if (!ValidateDataLayout(buffer))
                {
                    mHeader = nullptr;
                    mVertices = nullptr;
                    mIndices = nullptr;
                }
            }

            const Header* mHeader{};
            const V* mVertices{};
            const I* mIndices{};
        };

    }
    class RenderGeometries : public CacheWatcher<io::Buffer>
    {
    public:
        RenderGeometries(ID3D12Device* device, io::VirtualTransportSystem& vts, io::VirtualTransportSystem::Section fileName);
        ~RenderGeometries();

        io::Buffer GetGeometry(const io::Tag& tag);
        std::vector<io::Buffer> GetGeometries(const io::Tags& tags);

        void Preload(const io::Tags& tags);

        static void PopulateMappings(io::VirtualTransportSystem::Section fileName, io::VirtualTransportSystem& vts);
        static void SaveMappings(io::VirtualTransportSystem::Section fileName, io::VirtualTransportSystem& vts);

    private:
        io::Buffer LoadGeometry(const io::Tag& tag);
        ID3D12Device* mDevice = {};
    };


    //-------------------------------------------------------------------------------------------------
    class GeometriesResources
    {
    public:
        GeometriesResources(DeviceB& device, RenderGeometries& renderTextures);
        ~GeometriesResources();

        struct GeometryData
        {
            geom::Header mHeader{};

            ID3D12Resource* mVerticesResource{};
            ID3D12Resource* mIndicesResource{};
        };

        GeometryData GetResource(const io::Tag& tag);
        std::vector<GeometryData> GetResources(const io::Tags& tags);

        void Preload(const io::Tags& tags);

    private:
        DeviceB& mDevice;
        RenderGeometries& mRenderGeometries;

        struct ResourceData
        {
            geom::Header mHeader{};

            unique_obj<D3D12MA::Allocation> mVerticesAllocation;
            ComPtr<ID3D12Resource> mVerticesResource;

            unique_obj<D3D12MA::Allocation> mIndicesAllocation;
            ComPtr<ID3D12Resource> mIndicesResource;
        };

        using ResourceMap = std::map<io::Tag, ResourceData>;
        ResourceMap mResources;

        std::shared_mutex mSharedMutex;
    };

}
