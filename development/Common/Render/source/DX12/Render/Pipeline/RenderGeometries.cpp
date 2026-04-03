#include "Render/Device.h"
#include "Render/Pipeline/RenderGeometries.h"
#include "Render/Pipeline/RenderTextures.h"
#include "Render/Platform/D3D12MemAlloc.h"
#include "Render/RenderStringHelpers.h"
#include "Render/Platform/Adapter.h"
#include "Render/Helpers/ResourceDescriptions.h"
#include "Render\PlaceholderAssets\PlaceholderAssets.h"

#include <d3dx12.h>
#include <VertexTypes.h>


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
    bool ValidateGeometryFile(const yaget::Strings& /*stringsAsset*/)
    {
        return true;
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
    template<typename I>
    std::vector<I> GetIndices(const yaget::Strings& stringsAsset)
    {
        std::vector<I> result;
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
                        result.emplace_back(yaget::conv::FromString<I>(token.c_str()));
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
        header.mVertexFormatSize = sizeof(V::value_type);
        header.mIndexFormatSize = sizeof(I::value_type);
        header.mNumVertices = vertices.size();
        header.mNumIndices = indices.size();

        yaget::io::MessagingBuffer messagingBuffer(bufferSize);
        messagingBuffer.WriteDataChunk(&signature, sizeof(signature));
        messagingBuffer.WriteDataChunk(&header, sizeof(header));

        messagingBuffer.WriteDataChunk(static_cast<const void*>(vertices.data()), vertices.size() * sizeof(V::value_type));
        messagingBuffer.WriteDataChunk(static_cast<const void*>(indices.data()), indices.size() * sizeof(I::value_type));

        return messagingBuffer.mBuffer;
    }


    //-------------------------------------------------------------------------------------------------
    struct GeometryResourceResult
    {
        yaget::render::helpers::GpuResourceResult mVertices;
        yaget::render::helpers::GpuResourceResult mIndices;
    };


    //-------------------------------------------------------------------------------------------------
    template<typename VF>
    GeometryResourceResult CreateGeometryResource(const yaget::io::Tag& tag, const yaget::io::Buffer& buffer, ID3D12GraphicsCommandList* commandList, D3D12MA::Allocator* allocator)
    {
        using namespace yaget;

        render::geom::DataLayout<VF> dataLayout(buffer);

        size_t verticesBufferSize = dataLayout.mHeader->VertexBufferSize();
        render::helpers::SourceGpuParameters verticesGpuParameters = render::helpers::MakeVertexBufferParameters(reinterpret_cast<const uint8_t*>(dataLayout.mVertices), verticesBufferSize);

        render::helpers::GpuResourceResult gpuVerticesResult = render::helpers::CreateGpuResource(tag, verticesGpuParameters, commandList, allocator);

        render::helpers::GpuResourceResult gpuIndicesResult{};
        if (dataLayout.mHeader->mNumIndices && dataLayout.mIndices)
        {
            size_t indicesBufferSize = dataLayout.mHeader->IndexBufferSize();
            render::helpers::SourceGpuParameters indicesGpuParameters = render::helpers::MakeIndexBufferParameters(reinterpret_cast<const uint8_t*>(dataLayout.mIndices), indicesBufferSize);

            gpuIndicesResult = render::helpers::CreateGpuResource(tag, indicesGpuParameters, commandList, allocator);
        }

        return { 
            .mVertices ={ .mGpuAllocation = gpuVerticesResult.mGpuAllocation, .mUploadAllocation = gpuVerticesResult.mUploadAllocation },
            .mIndices ={ .mGpuAllocation = gpuIndicesResult.mGpuAllocation, .mUploadAllocation = gpuIndicesResult.mUploadAllocation }
        };
    }


    //-------------------------------------------------------------------------------------------------
    template<typename V, typename I>
    yaget::io::Buffer GenerateGeometryBuffer(yaget::render::AssetCacheType vertexFormat, const yaget::Strings& strings)
    {
        auto vertices = GetVertices<V>(strings);
        auto indices = GetIndices<I>(strings);

        auto buffer = SerializeToBuffer(vertexFormat, vertices, indices);

        return buffer;
    }


    //-------------------------------------------------------------------------------------------------
    // simple test for mapping vertex type to actual type
    template <yaget::render::AssetCacheType T>
    struct VertexTypeResolver;


    //-------------------------------------------------------------------------------------------------
    template <>
    struct VertexTypeResolver<yaget::render::AssetCacheType::VertexPosition | yaget::render::AssetCacheType::VertexColor | yaget::render::AssetCacheType::VertexTexture0>
    {
        using Type = DirectX::VertexPositionColorTexture;
    };


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
    auto stringsAsset = loader.GetAsset();
    const Strings* geometryFile{};

    if (stringsAsset && stringsAsset->IsValid() && ValidateGeometryFile(stringsAsset->mStrings))
    {
        geometryFile = &stringsAsset->mStrings;
    }
    else
    {
        YLOG_ERROR("REND", "Could not find geometry data for tag: '%s', replacing with built-in placeholder.", yaget::conv::ToString(tag).c_str());

        geometryFile = &placeholders::GetGeometryData();
    }

    const auto vertexFormat = GetVertexFormat(*geometryFile);

    if (vertexFormat == AssetCacheType::VertexPosition)
    {
        buffer = GenerateGeometryBuffer<DirectX::VertexPosition, uint32_t>(vertexFormat, *geometryFile);
    }
    else if (vertexFormat == (AssetCacheType::VertexPosition | AssetCacheType::VertexColor))
    {
        buffer = GenerateGeometryBuffer<DirectX::VertexPositionColor, uint32_t>(vertexFormat, *geometryFile);
    }
    else if (vertexFormat == (AssetCacheType::VertexPosition | AssetCacheType::VertexColor | AssetCacheType::VertexTexture0))
    {
        buffer = GenerateGeometryBuffer<DirectX::VertexPositionColorTexture, uint32_t>(vertexFormat, *geometryFile);
    }
    else if (vertexFormat == (AssetCacheType::VertexPosition | AssetCacheType::VertexTexture0))
    {
        buffer = GenerateGeometryBuffer<DirectX::VertexPositionTexture, uint32_t>(vertexFormat, *geometryFile);
    }
    else
    {
        YAGET_ASSERT(false, "Geometry: '%s' Vertex Format: '%s' is not handled!!!", yaget::conv::ToString(tag).c_str(), conv::ToString(vertexFormat).c_str());
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
std::vector<yaget::render::GeometriesResources::GeometryData> yaget::render::GeometriesResources::GetResources(const io::Tags& tags)
{
    std::vector<GeometryData> results;
    io::Tags tagsToLoad;
    {
        mt::ReadLock readLocker(mSharedMutex);
        for (const auto& tag : tags)
        {
            if (auto it = mResources.find(tag); it != mResources.end())
            {
                GeometryData geometryData
                {
                    .mHeader = it->second.mHeader,
                    .mVerticesResource = it->second.mVerticesResource.Get(),
                    .mIndicesResource = it->second.mIndicesResource.Get()
                };

                results.push_back(geometryData);
                continue;
            }

            tagsToLoad.push_back(tag);
        }

        if (tagsToLoad.empty())
        {
            return results;
        }
    }

    std::vector<unique_obj<D3D12MA::Allocation>> allocationsToKeepAlive;
    auto framerHandler = mDevice.GetCopyCommands();
    auto commandList = framerHandler.BeginFrame(nullptr);
    auto preloadCommandList = commandList->GetDeviceCommandList();

    mt::WriteLock writeLocker(mSharedMutex);
    for (const auto& tag : tagsToLoad)
    {
        // it's possible that between the time we released read lock and acquired write lock, 
        // another thread loaded the same texture, so we need to check again if resource is already in map
        if (auto it = mResources.find(tag); it != mResources.end())
        {
            const auto& element = it->second;
            GeometryData geometryData
            {
                .mHeader = element.mHeader,
                .mVerticesResource = element.mVerticesResource.Get(),
                .mIndicesResource = element.mIndicesResource.Get()
            };
            results.push_back(geometryData);
            continue;
        }

        auto geometryBuffer = mRenderGeometries.GetGeometry(tag);
        if (!io::size_data(geometryBuffer))
        {
            // this should not happen here, since the mRenderGeometries.GetGeometry weill always return valid buffer.
            // It will use placeholder geometry if tag does not exist.
            YAGET_ASSERT(false, "Could not find geometry data for tag: '%s'.", yaget::conv::ToString(tag).c_str());
            continue;
        }

        if (const geom::Header* header = geom::GetHeader(geometryBuffer))
        {
            auto allocator = mDevice.GetAdapter().GetAllocator();

            auto vertexFormat = header->mVertexFormat;

            GeometryResourceResult resourceResult{};

            if (vertexFormat == AssetCacheType::VertexPosition)
            {
                resourceResult = CreateGeometryResource<DirectX::VertexPosition>(tag, geometryBuffer, preloadCommandList, allocator);
            }
            else if (vertexFormat == (AssetCacheType::VertexPosition | AssetCacheType::VertexColor))
            {
                resourceResult = CreateGeometryResource<DirectX::VertexPositionColor>(tag, geometryBuffer, preloadCommandList, allocator);
            }
            else if (vertexFormat == (AssetCacheType::VertexPosition | AssetCacheType::VertexColor | AssetCacheType::VertexTexture0))
            {
                resourceResult = CreateGeometryResource<DirectX::VertexPositionColorTexture>(tag, geometryBuffer, preloadCommandList, allocator);
            }
            else if (vertexFormat == (AssetCacheType::VertexPosition | AssetCacheType::VertexTexture0))
            {
                resourceResult = CreateGeometryResource<DirectX::VertexPositionTexture>(tag, geometryBuffer, preloadCommandList, allocator);
            }
            else
            {
                YAGET_ASSERT(false, "Geometry: '%s' Vertex Format: '%s' is not handled!!!", yaget::conv::ToString(tag).c_str(), conv::ToString(vertexFormat).c_str());
            }

            //-------------------------------------------------------------------------------------------------
            // extract vertex data
            allocationsToKeepAlive.push_back(unique_obj<D3D12MA::Allocation>{ resourceResult.mVertices.mUploadAllocation });
            D3D12MA::Allocation* verticesGpuAllocation = resourceResult.mVertices.mGpuAllocation;

            ComPtr<ID3D12Resource> verticesGpuGeometry = verticesGpuAllocation->GetResource();
            unique_obj<D3D12MA::Allocation> verticesAllocation(verticesGpuAllocation);

            //-------------------------------------------------------------------------------------------------
            // extract index data
            unique_obj<D3D12MA::Allocation> indicesAllocation;
            ComPtr<ID3D12Resource> indicesGpuGeometry;

            if (header->mNumIndices)
            {
                allocationsToKeepAlive.push_back(unique_obj<D3D12MA::Allocation>{ resourceResult.mIndices.mUploadAllocation });
                D3D12MA::Allocation* indicesGpuAllocation = resourceResult.mIndices.mGpuAllocation;

                indicesGpuGeometry = indicesGpuAllocation->GetResource();
                indicesAllocation.reset(indicesGpuAllocation);
            }

            ResourceData resourceData
            {
                .mHeader = *header,
                .mVerticesAllocation = std::move(verticesAllocation),
                .mVerticesResource = verticesGpuGeometry,

                .mIndicesAllocation = std::move(indicesAllocation),
                .mIndicesResource = indicesGpuGeometry
            };

            if (!resourceData.mIndicesResource)
            {
                resourceData.mHeader.mNumIndices = 0;
            }

            GeometryData geometryData
            {
                .mHeader = resourceData.mHeader,
                .mVerticesResource = resourceData.mVerticesResource.Get(),
                .mIndicesResource = resourceData.mIndicesResource.Get()
            };

            mResources.insert({ tag, std::move(resourceData) });
            results.push_back(geometryData);
        }
    }

    framerHandler.EndFrame();

    return results;
}


//-------------------------------------------------------------------------------------------------
void yaget::render::GeometriesResources::Preload(const io::Tags& tags)
{
    GetResources(tags);
}


//-------------------------------------------------------------------------------------------------
yaget::render::GeometriesResources::GeometryData yaget::render::GeometriesResources::GetResource(const io::Tag& tag)
{
    auto resources = GetResources(io::Tags{ tag });
    return !resources.empty() ? *resources.begin() : GeometryData{};
}
