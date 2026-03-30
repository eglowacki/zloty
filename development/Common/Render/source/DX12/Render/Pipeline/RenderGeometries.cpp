#include "Render/Device.h"
#include "Render/Pipeline/RenderGeometries.h"
#include "Render/Pipeline/RenderTextures.h"
#include "Render/Platform/D3D12MemAlloc.h"
#include "Render/RenderStringHelpers.h"
#include "Render/Platform/DeviceDebugger.h"
#include "Core/ErrorHandlers.h"

#include <complex.h>
#include <d3dx12.h>
#include <VertexTypes.h>

#include "Render/Platform/Adapter.h"


namespace
{
    size_t GeometryBufferVersion = 1;
    //-------------------------------------------------------------------------------------------------
    yaget::render::AssetCacheType GetVertexFormat(const yaget::Strings& stringsAsset)
    {
        using namespace yaget::render;

        for (const auto& line : stringsAsset)
        {
            if (line.find("VertexFormat:") != std::string::npos)
            {
                auto vertexFormatStr = line.substr(std::string("VertexFormat:").size());
                yaget::conv::Trim(vertexFormatStr);
                auto vertexFormat = yaget::conv::FromString<AssetCacheType>(vertexFormatStr.c_str());
                return vertexFormat;
            }
        }

        return AssetCacheType::Empty;
    }


    //-------------------------------------------------------------------------------------------------
    bool IsLineValidVertex(const std::string& line, int numComponents)
    {
        if (line.find("//") != std::string::npos)
        {
            return false;
        }

        const auto numLeftBraces = std::ranges::count(line, '{');
        const auto numRightBraces = std::ranges::count(line, '}');
        return numLeftBraces == numComponents && numRightBraces == numComponents;
    }


    //-------------------------------------------------------------------------------------------------
    template<typename T>
    std::vector<T> GetVertices(const yaget::Strings& stringsAsset)
    {
        std::vector<T> result;
        const auto numComponents = T::InputLayout.NumElements;
        auto it = std::ranges::find_if(stringsAsset, [](const auto& element)
        {
            return element.find("Vertices Begin:") != std::string::npos;
        });

        for (; it != stringsAsset.end(); ++it)
        {
            const auto& line = *it;

            if (line.find("Vertices End:") != std::string::npos)
            {
                break;
            }
            else if (IsLineValidVertex(line, numComponents))
            {
                auto vertexData = yaget::conv::FromString<T>(line.c_str());
                result.push_back(vertexData);
            }
        }

        return result;
    }


    //-------------------------------------------------------------------------------------------------
    std::vector<uint32_t> GetIndices(const yaget::Strings& stringsAsset)
    {
        std::vector<uint32_t> result;
        auto it = std::ranges::find_if(stringsAsset, [](const auto& element)
        {
            return element.find("Indices Begin:") != std::string::npos;
        });

        if (it != stringsAsset.end())
        {
            ++it;
        }

        for (; it != stringsAsset.end(); ++it)
        {
            const auto& line = *it;

            if (line.find("Indices End:") != std::string::npos)
            {
                break;
            }
            else if (line.find("//") == std::string::npos)
            {
                const auto tokens = yaget::conv::Split(line, ",", true);
                for (const auto& token : tokens)
                {
                    if (!token.empty())
                    {
                        result.emplace_back(yaget::conv::FromString<uint32_t>(token.c_str()));
                    }
                }
            }
        }

        return result;
    }


    //-------------------------------------------------------------------------------------------------
    template<typename V, typename I>
    yaget::io::Buffer SerializeToBuffer(yaget::render::AssetCacheType vertexFormat, const V& vertices, const I& indices)
    {
        size_t bufferSize = yaget::render::geom::HeaderBufferSize + vertices.size() * sizeof(V::value_type) + indices.size() * sizeof(I::value_type);

        yaget::render::YagetFileSignature signature;
        signature.Version = GeometryBufferVersion;

        yaget::render::geom::Header header;
        header.mVertexFormat = vertexFormat;
        header.mNumVertices = vertices.size();
        header.mNumIndices = indices.size();

        yaget::io::MessagingBuffer messagingBuffer(bufferSize);
        messagingBuffer.WriteDataChunk(&signature, sizeof(signature));
        messagingBuffer.WriteDataChunk(&header, sizeof(header));

        messagingBuffer.WriteDataChunk(static_cast<const void*>(vertices.data()), vertices.size() * sizeof(V::value_type));
        messagingBuffer.WriteDataChunk(static_cast<const void*>(indices.data()), indices.size() * sizeof(I::value_type));

        return messagingBuffer.mBuffer;
    }

    template<typename VF>
    D3D12MA::Allocation* CreateGeometryResource(D3D12MA::Allocator* allocator, const yaget::io::Buffer& buffer, const yaget::io::Tag& assetTag)
    {
        yaget::render::geom::DataLayout<VF> dataLayout(buffer);

        size_t bufferSize = sizeof(VF) * dataLayout.mHeader->mNumVertices;
        D3D12_RESOURCE_DESC resourceDesc = {};
        resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        resourceDesc.Alignment = 0;
        resourceDesc.Width = bufferSize;
        resourceDesc.Height = 1;
        resourceDesc.DepthOrArraySize = 1;
        resourceDesc.MipLevels = 1;
        resourceDesc.Format = DXGI_FORMAT_UNKNOWN;
        resourceDesc.SampleDesc.Count = 1;
        resourceDesc.SampleDesc.Quality = 0;
        resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        resourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

        D3D12MA::ALLOCATION_DESC allocationDesc = {};
        allocationDesc.HeapType = D3D12_HEAP_TYPE_UPLOAD;

        D3D12MA::Allocation* allocation = nullptr;
        yaget::render::ComPtr<ID3D12Resource> geometryResource;
        HRESULT hr = allocator->CreateResource(
            &allocationDesc,
            &resourceDesc,
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            &allocation,
            IID_PPV_ARGS(&geometryResource));
        yaget::error_handlers::ThrowOnError(hr, "Could not CreateResource from allocator.");

        yaget::render::platform::SetDebugName(geometryResource.Get(), allocation, "Geometry", yaget::conv::ToString(assetTag));

        void* bufferData = nullptr;
        hr = geometryResource->Map(0, nullptr, &bufferData);
        yaget::error_handlers::ThrowOnError(hr, "Could not map RenderShape buffer for write.");
        
        memcpy(bufferData, dataLayout.mVertices, bufferSize);
        geometryResource->Unmap(0, nullptr);

        return allocation;
    }

}


//-------------------------------------------------------------------------------------------------
bool yaget::render::geom::ValidateDataLayout(const io::Buffer& buffer)
{
    const YagetFileSignature* signature = io::cast_data<const YagetFileSignature>(buffer);
    if (!signature->IsValid(GeometryBufferVersion))
    {
        YLOG_ERROR("DEVI", "Invalid geometry buffer signature '%d'. Expected 'GLOW' with version <= %d.", signature->Version, GeometryBufferVersion);
        return false;
    }

    return true;
}


//-------------------------------------------------------------------------------------------------
yaget::render::RenderGeometries::RenderGeometries(ID3D12Device* device, io::VirtualTransportSystem& vts, io::VirtualTransportSystem::Section fileName)
    : CacheWatcher(vts, fileName)
    , mDevice(device)
{
}


//-------------------------------------------------------------------------------------------------
yaget::render::RenderGeometries::~RenderGeometries() = default;


//-------------------------------------------------------------------------------------------------
yaget::io::Buffer yaget::render::RenderGeometries::GetGeometry(const io::Tag& tag)
{
    YAGET_ASSERT(tag.IsValid(), "Tag: '%s:%s' is not valid.",
                 yaget::conv::ToString(tag.mGuid).c_str(),
                 yaget::conv::ToString(tag).c_str());

    auto result = GetGeometries(io::Tags{ tag });
    return !result.empty() ? *result.begin() : io::Buffer{};
}


//-------------------------------------------------------------------------------------------------
std::vector<yaget::io::Buffer> yaget::render::RenderGeometries::GetGeometries(const io::Tags& tags)
{
    std::vector<io::Buffer> results;

    std::lock_guard mutexLocker(mMutex);

    for (auto tag : tags)
    {
        auto result = GetAsset(tag, [this, &results](const auto& tag, auto& cachedData)
        {
            if (!io::size_data(cachedData))
            {
                cachedData = LoadGeometry(tag);
            }

            return cachedData;
        });

        results.push_back(result);
    }

    return results;
}


//-------------------------------------------------------------------------------------------------
void yaget::render::RenderGeometries::Preload(const io::Tags& tags)
{
    GetGeometries(tags);
}


//-------------------------------------------------------------------------------------------------
void yaget::render::RenderGeometries::PopulateMappings(io::VirtualTransportSystem::Section /*fileName*/, io::VirtualTransportSystem& /*vts*/)
{
}


//-------------------------------------------------------------------------------------------------
void yaget::render::RenderGeometries::SaveMappings(io::VirtualTransportSystem::Section /*fileName*/, io::VirtualTransportSystem& /*vts*/)
{
}


//-------------------------------------------------------------------------------------------------
yaget::io::Buffer yaget::render::RenderGeometries::LoadGeometry(const io::Tag& tag)
{
    io::Buffer buffer;

    io::SingleBLobLoader<io::StringsAsset> loader(mVTS, tag);
    if (auto stringsAsset = loader.GetAsset(); stringsAsset->IsValid())
    {
        const auto vertexFormat = GetVertexFormat(stringsAsset->mStrings);

        if (vertexFormat == AssetCacheType::VertexPosition)
        {
            auto vertices = GetVertices<DirectX::VertexPosition>(stringsAsset->mStrings);
            auto indices= GetIndices(stringsAsset->mStrings);

            buffer = SerializeToBuffer(vertexFormat, vertices, indices);
        }
        else if (vertexFormat == (AssetCacheType::VertexPosition | AssetCacheType::VertexColor))
        {
            auto vertices = GetVertices<DirectX::VertexPositionColor>(stringsAsset->mStrings);
            auto indices= GetIndices(stringsAsset->mStrings);

            buffer = SerializeToBuffer(vertexFormat, vertices, indices);
        }
        else if (vertexFormat == (AssetCacheType::VertexPosition | AssetCacheType::VertexColor | AssetCacheType::VertexTexture0))
        {
            auto vertices = GetVertices<DirectX::VertexPositionColorTexture>(stringsAsset->mStrings);
            auto indices= GetIndices(stringsAsset->mStrings);

            buffer = SerializeToBuffer(vertexFormat, vertices, indices);
        }
        else if (vertexFormat == (AssetCacheType::VertexPosition | AssetCacheType::VertexTexture0))
        {
            auto vertices = GetVertices<DirectX::VertexPositionTexture>(stringsAsset->mStrings);
            auto indices= GetIndices(stringsAsset->mStrings);

            buffer = SerializeToBuffer(vertexFormat, vertices, indices);
        }
        else
        {
            YAGET_ASSERT(false, "Geometry: '%s' Vertex Format: '%s' is not handled!!!", yaget::conv::ToString(tag).c_str(), conv::ToString(vertexFormat).c_str());
        }
    }

    return buffer;
}


//-------------------------------------------------------------------------------------------------
yaget::render::GeometriesResources::GeometriesResources(DeviceB& device, RenderGeometries& renderGeometries)
    : mDevice(device)
    , mRenderGeometries(renderGeometries)
{
}


//-------------------------------------------------------------------------------------------------
yaget::render::GeometriesResources::~GeometriesResources() = default;


//-------------------------------------------------------------------------------------------------
std::vector<yaget::render::ComPtr<ID3D12Resource>> yaget::render::GeometriesResources::GetResources(const io::Tags& tags)
{
    std::vector<ComPtr<ID3D12Resource>> results;

    for (const auto& tag : tags)
    {
        {
            mt::ReadLock readLocker(mSharedMutex);
            if (auto it = mResources.find(tag); it != mResources.end())
            {
                results.push_back(it->second.mResource);
                continue;
            }
        }

        auto geometryBuffer = mRenderGeometries.GetGeometry(tag);
        if (!io::size_data(geometryBuffer))
        {
            // NOTE(eg) Should we return some kind of built-in geometry placeholder, 
            // in similar manner as we do it in RenderShaders (missing vertex or pixel shader)
            YLOG_ERROR("REND", "Could not find geometry data for tag: '%s'.", yaget::conv::ToString(tag).c_str());
            continue;
        }

        mt::WriteLock writeLocker(mSharedMutex);

        if (const geom::Header* header = geom::GetHeader(geometryBuffer))
        {
            auto allocator = mDevice.GetAdapter().GetAllocator();
            D3D12MA::Allocation* allocation = nullptr;

            auto vertexFormat = header->mVertexFormat;

            if (vertexFormat == AssetCacheType::VertexPosition)
            {
                allocation = CreateGeometryResource<DirectX::VertexPosition>(allocator, geometryBuffer, tag);
            }
            else if (vertexFormat == (AssetCacheType::VertexPosition | AssetCacheType::VertexColor))
            {
                allocation = CreateGeometryResource<DirectX::VertexPositionColor>(allocator, geometryBuffer, tag);
            }
            else if (vertexFormat == (AssetCacheType::VertexPosition | AssetCacheType::VertexColor | AssetCacheType::VertexTexture0))
            {
                allocation = CreateGeometryResource<DirectX::VertexPositionColorTexture>(allocator, geometryBuffer, tag);
            }
            else if (vertexFormat == (AssetCacheType::VertexPosition | AssetCacheType::VertexTexture0))
            {
                allocation = CreateGeometryResource<DirectX::VertexPositionTexture>(allocator, geometryBuffer, tag);
            }
            else
            {
                YAGET_ASSERT(false, "Geometry: '%s' Vertex Format: '%s' is not handled!!!", yaget::conv::ToString(tag).c_str(), conv::ToString(vertexFormat).c_str());
            }

            ComPtr<ID3D12Resource> geometry = allocation->GetResource();
            unique_obj<D3D12MA::Allocation> geometryAllocation(allocation);
            mResources.insert({ tag, ResourceData{ std::move(geometryAllocation), geometry } });

            results.push_back(geometry);
        }
    }

    return results;
}


//-------------------------------------------------------------------------------------------------
void yaget::render::GeometriesResources::Preload(const io::Tags& tags)
{
    GetResources(tags);
}


//-------------------------------------------------------------------------------------------------
yaget::render::ComPtr<ID3D12Resource> yaget::render::GeometriesResources::GetResource(const io::Tag& tag)
{
    auto resources = GetResources(io::Tags{ tag });
    return !resources.empty() ? *resources.begin() : ComPtr<ID3D12Resource>{};
}
