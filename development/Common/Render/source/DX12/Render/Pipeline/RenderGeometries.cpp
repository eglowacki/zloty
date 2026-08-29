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
    struct GeometryResourceResult
    {
        yaget::render::helpers::GpuResourceResult mVertices;
        yaget::render::helpers::GpuResourceResult mIndices;
    };


    //-------------------------------------------------------------------------------------------------
    template<typename T>
    void UpdateResourceBuffer(ID3D12Resource* resource, const T* data, size_t dataSize)
    {
        D3D12_RANGE emptyRange = { 0, 0 };
        void* mappedPtr = nullptr;
        HRESULT hr = resource->Map(0, &emptyRange, &mappedPtr);
        if (FAILED(hr))
        {
            auto debugName = YAGET_RENDER_GET_DEBUG_NAME(resource);
            YLOG_ERROR("REND", "Did not mapped resource '%s' for updating data.", debugName.c_str());

            return;
        }

        memcpy(mappedPtr, reinterpret_cast<const uint8_t*>(data), dataSize);
        resource->Unmap(0, nullptr);
    }


    //-------------------------------------------------------------------------------------------------
    template<typename VF>
    GeometryResourceResult CreateGeometryResource(const yaget::io::Tag& tag, const yaget::io::Buffer& buffer, ID3D12GraphicsCommandList* commandList, D3D12MA::Allocator* allocator)
    {
        using namespace yaget;

        render::geom::DataLayout<VF> dataLayout(buffer);

        size_t verticesBufferSize = dataLayout.mHeader->VertexBufferSize();

        if (dataLayout.mHeader->mUpdateType == render::geom::Header::UpdateType::CpuUpload)
        {
            D3D12MA::Allocation* verticesAllocation = render::helpers::CreateUploadHeap(tag, verticesBufferSize, allocator);
            ID3D12Resource* verticesResource = verticesAllocation->GetResource();

            UpdateResourceBuffer(verticesResource, dataLayout.mVertices, verticesBufferSize);

            GeometryResourceResult results;
            results.mVertices.mGpuAllocation = verticesAllocation;

            if (dataLayout.mHeader->mNumIndices && dataLayout.mIndices)
            {
                size_t indicesBufferSize = dataLayout.mHeader->IndexBufferSize();
                D3D12MA::Allocation* indicesAllocation = render::helpers::CreateUploadHeap(tag, indicesBufferSize, allocator);
                ID3D12Resource* indicesResource = indicesAllocation->GetResource();

                UpdateResourceBuffer(indicesResource, dataLayout.mIndices, indicesBufferSize);

                results.mIndices.mGpuAllocation = indicesAllocation;
            }

            return results;
        }
        else
        {
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
                .mVertices = {.mGpuAllocation = gpuVerticesResult.mGpuAllocation, .mUploadAllocation = gpuVerticesResult.mUploadAllocation },
                .mIndices = {.mGpuAllocation = gpuIndicesResult.mGpuAllocation, .mUploadAllocation = gpuIndicesResult.mUploadAllocation }
            };
        }
    }


    //-------------------------------------------------------------------------------------------------
    template<typename V, typename I>
    yaget::io::Buffer GenerateGeometryBuffer(yaget::render::AssetCacheType vertexFormat, const yaget::Strings& strings)
    {
        auto vertices = GetVertices<V>(strings);
        auto indices = GetIndices<I>(strings);

        auto buffer = SerializeToBuffer(vertexFormat, vertices, indices, yaget::render::geom::Header::UpdateType::GpuUpload);

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
    if (!signature->IsValid(GeometriesResources::GeometryBufferVersion))
    {
        YLOG_ERROR("DEVI", "Invalid geometry buffer signature '%d'. Expected 'GLOW' with version <= %d.", signature->Version, GeometriesResources::GeometryBufferVersion);
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

    auto result = GetGeometries(io::Tags{ tag }, nullptr);
    return !result.empty() ? *result.begin() : io::Buffer{};
}


//-------------------------------------------------------------------------------------------------
std::vector<yaget::io::Buffer> yaget::render::RenderGeometries::GetGeometries(const io::Tags& tags, comp::gs::mt::InitCounter* counter)
{
    std::vector<io::Buffer> results;

    std::lock_guard mutexLocker(mMutex);

    for (auto tag : tags)
    {
        auto result = GetAsset(tag, [this, &results, counter](const auto& tag, auto& cachedData)
        {
            if (!io::size_data(cachedData))
            {
                cachedData = LoadGeometry(tag);
            }

            if (counter)
            {
                ++(*counter);
            }

            return cachedData;
        });

        results.push_back(result);
    }

    return results;
}


//-------------------------------------------------------------------------------------------------
void yaget::render::RenderGeometries::Preload(const io::Tags& tags, comp::gs::mt::InitCounter& counter)
{
    GetGeometries(tags, &counter);
}


//-------------------------------------------------------------------------------------------------
void yaget::render::RenderGeometries::AttachGeometry(const io::Tag& tag, io::Buffer buffer)
{
    std::lock_guard mutexLocker(mMutex);
    mAssets[tag] = buffer;
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
size_t yaget::render::GeometriesResources::GeometryBufferVersion = 1;


//-------------------------------------------------------------------------------------------------
yaget::render::GeometriesResources::GeometriesResources(DeviceB& device, RenderGeometries& renderGeometries)
    : mDevice(device)
    , mRenderGeometries(renderGeometries)
{
}


//-------------------------------------------------------------------------------------------------
yaget::render::GeometriesResources::~GeometriesResources() = default;


//-------------------------------------------------------------------------------------------------
yaget::render::GeometriesResources::Geometries yaget::render::GeometriesResources::GetResources(const io::Tags& tags, comp::gs::mt::InitCounter* counter)
{
    std::vector<GeometryData> results;
    io::Tags tagsToLoad;
    {
        mt::ReadLock readLocker(mSharedMutex);
        for (const auto& tag : tags)
        {
            if (auto geometryData = FindGeometryData(tag); geometryData.mHeader.IsValid())
            {
                results.push_back(geometryData);
                if (counter)
                {
                    ++(*counter);
                }
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
    auto commandList = framerHandler.BeginFrame(nullptr, nullptr);
    auto preloadCommandList = commandList->GetDeviceCommandList();

    mt::WriteLock writeLocker(mSharedMutex);
    for (const auto& tag : tagsToLoad)
    {
        // it's possible that between the time we released read lock and acquired write lock, 
        // another thread loaded the same texture, so we need to check again if resource is already in map
        if (auto geometryData = FindGeometryData(tag); geometryData.mHeader.IsValid())
        {
            results.push_back(geometryData);
            if (counter)
            {
                ++(*counter);
            }
            continue;
        }

        auto geometryBuffer = mRenderGeometries.GetGeometry(tag);
        if (!io::size_data(geometryBuffer))
        {
            // this should not happen here, since the mRenderGeometries.GetGeometry will always return valid buffer.
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

            auto useGpuUpload = header->mUpdateType == render::geom::Header::UpdateType::GpuUpload;

            //-------------------------------------------------------------------------------------------------
            // extract vertex data
            if (useGpuUpload)
            {
                allocationsToKeepAlive.push_back(unique_obj<D3D12MA::Allocation>{ resourceResult.mVertices.mUploadAllocation });
            }
            D3D12MA::Allocation* verticesGpuAllocation = resourceResult.mVertices.mGpuAllocation;

            ComPtr<ID3D12Resource> verticesGpuGeometry = verticesGpuAllocation->GetResource();
            unique_obj<D3D12MA::Allocation> verticesAllocation(verticesGpuAllocation);

            //-------------------------------------------------------------------------------------------------
            // extract index data
            unique_obj<D3D12MA::Allocation> indicesAllocation;
            ComPtr<ID3D12Resource> indicesGpuGeometry;

            if (header->mNumIndices)
            {
                if (useGpuUpload)
                {
                    allocationsToKeepAlive.push_back(unique_obj<D3D12MA::Allocation>{ resourceResult.mIndices.mUploadAllocation });
                }
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
            if (counter)
            {
                ++(*counter);
            }
        }
    }

    framerHandler.EndFrame();

    return results;
}


//-------------------------------------------------------------------------------------------------
bool yaget::render::GeometriesResources::UpdateResourceData(const io::Tag& tag, const io::Buffer& buffer)
{
    {
        mt::WriteLock writeLocker(mSharedMutex);

        mRenderGeometries.AttachGeometry(tag, buffer);
        geom::DataLayout<uint8_t, uint32_t> dataLayout(buffer);

        if (auto geometryData = FindGeometryData(tag); geometryData.mHeader.IsValid() && dataLayout.mHeader->mUpdateType == geom::Header::UpdateType::CpuUpload)
        {
            const bool sameVertexFormat = geometryData.mHeader.mVertexFormatSize == dataLayout.mHeader->mVertexFormatSize;
            const bool verticesSizeFits = geometryData.mHeader.mNumVertices >= dataLayout.mHeader->mNumVertices;
            const bool sameIndexFormat = geometryData.mHeader.mIndexFormatSize == dataLayout.mHeader->mIndexFormatSize;
            const bool indicesSizeFits = geometryData.mHeader.mNumIndices >= dataLayout.mHeader->mNumIndices;

            if (sameVertexFormat && verticesSizeFits && sameIndexFormat && indicesSizeFits)
            {
                UpdateResourceBuffer(geometryData.mVerticesResource, dataLayout.mVertices, dataLayout.mHeader->VertexBufferSize());
                UpdateResourceBuffer(geometryData.mIndicesResource, dataLayout.mIndices, dataLayout.mHeader->IndexBufferSize());

                mResources[tag].mHeader = *dataLayout.mHeader;

                return true;
            }
        }
    }

    ClearResource(tag);
    return false;
}


//-------------------------------------------------------------------------------------------------
void yaget::render::GeometriesResources::ClearResource(const io::Tag& tag)
{
    mt::WriteLock writeLocker(mSharedMutex);
    mResources.erase(tag);
}


//-------------------------------------------------------------------------------------------------
void yaget::render::GeometriesResources::Preload(const io::Tags& tags, comp::gs::mt::InitCounter& counter)
{
    GetResources(tags, &counter);
}


//-------------------------------------------------------------------------------------------------
yaget::render::GeometriesResources::GeometryData yaget::render::GeometriesResources::FindGeometryData(const io::Tag& tag) const
{
    if (auto it = mResources.find(tag); it != mResources.end())
    {
        const auto& element = it->second;
        GeometryData geometryData
        {
            .mHeader = element.mHeader,
            .mVerticesResource = element.mVerticesResource.Get(),
            .mIndicesResource = element.mIndicesResource.Get()
        };

        return geometryData;
    }

    return {};
}


//-------------------------------------------------------------------------------------------------
yaget::render::GeometriesResources::GeometryData yaget::render::GeometriesResources::GetResource(const io::Tag& tag)
{
    auto resources = GetResources(io::Tags{ tag }, nullptr);
    return !resources.empty() ? *resources.begin() : GeometryData{};
}
