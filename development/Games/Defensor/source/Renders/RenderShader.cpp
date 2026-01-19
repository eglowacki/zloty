#include "RenderShader.h"

#include <CommonStates.h>
#include "Render/Platform/ResourceCompiler.h"
#include <d3dx12.h>
#include <VertexTypes.h>
#include "Streams/Guid.h"
#include "VTS/ResolvedAssets.h"


namespace
{
    const char* shaderSource = 
        R"( struct PSInput
            {
                float4 position : SV_POSITION;
                float4 color : COLOR;
            };

            PSInput VSMain(float4 position : SV_POSITION, float4 color : COLOR)
            {
                PSInput result;

                result.position = position;
                result.color = color;

                return result;
            }

            float4 PSMain(PSInput input) : SV_TARGET
            {
                return input.color;
            }
        )";

    // let's have a table of mapping between shader type to (entry point and target)
    struct ShaderMapping
    {
        const char* mEntryPoint;
        const char* mTarget;
    };

    std::map<defensor::render::RenderShader::ShaderType, ShaderMapping> ShaderMappings =
    {
        { defensor::render::RenderShader::ShaderType::Vertex, {"VSMain", "vs_5_0"} },
        { defensor::render::RenderShader::ShaderType::Pixel, {"PSMain", "ps_5_0"} },
        { defensor::render::RenderShader::ShaderType::Geometry, {"GSMain", "gs_5_0"} },
        { defensor::render::RenderShader::ShaderType::Compute, {"CSMain", "cs_5_0"} },
        { defensor::render::RenderShader::ShaderType::Hull, {"HSMain", "hs_5_0"} },
        { defensor::render::RenderShader::ShaderType::Domain, {"DSMain", "ds_5_0"} }
    };
  
}


//-------------------------------------------------------------------------------------------------
defensor::render::ShaderCache::ShaderCache(io::VirtualTransportSystem& vts, io::VirtualTransportSystem::Section fileName)
    : mVTS(vts)
    , mCacheSection(fileName)
{
    io::SingleBLobLoader<io::BinAsset> cacheLoader(mVTS, mCacheSection);
    if (auto asset = cacheLoader.GetAsset())
    {
        //auto dataPointer = io::cast_data<const char>(asset->mBuffer);
        //auto numElements = (static_cast<size_t*>(dataPointer));
        io::MessagingBuffer cache;
        cache.mBuffer = asset->mBuffer;
        cache.mWriteOffset = io::size_data(asset->mBuffer);

        auto numElements = *(reinterpret_cast<size_t*>(io::cast_data<char>(cache.mBuffer)));
        size_t offset = sizeof(numElements);
        for (size_t i = 0; i < numElements; ++i)
        {
            Guid *guid = reinterpret_cast<Guid*>(io::cast_data<char>(cache.mBuffer) + offset);
            offset += sizeof(Guid);
            Location *location = reinterpret_cast<Location*>(io::cast_data<char>(cache.mBuffer) + offset);
            offset += sizeof(Location);
            mCacheIndex.insert({ *guid, *location });
        }

        mCache = io::MessagingBuffer(io::size_data(cache.mBuffer) - offset);
        memcpy(io::cast_data<char>(mCache.mBuffer), io::cast_data<char>(cache.mBuffer) + offset, io::size_data(mCache.mBuffer));
    }

    //mCache - lod blob here
    //Caches@Shaders.cache
}


//-------------------------------------------------------------------------------------------------
defensor::render::ShaderCache::~ShaderCache()
{
    if (mCacheDirty)
    {
        // we need to serialize mCacheIndex and mCache into a single buffer and save it back to VTS
        io::Buffer indexBuffer = io::CreateBuffer(sizeof(size_t) + mCacheIndex.size() * (sizeof(Guid) + sizeof(Location)));
        auto dataPointer = io::cast_data<char>(indexBuffer);
        size_t offset = 0;
        auto numElements = mCacheIndex.size();
        std::memcpy(dataPointer + offset, &numElements, sizeof(numElements));
        offset += sizeof(numElements);
        for (const auto& [guid, location] : mCacheIndex)
        {
            std::memcpy(dataPointer + offset, &guid, sizeof(guid));
            offset += sizeof(guid);
            std::memcpy(dataPointer + offset, &location, sizeof(location));
            offset += sizeof(location);
        }

        auto fullCacheData = io::CreateBuffer(io::size_data(indexBuffer) + io::size_data(mCache.mBuffer));
        io::CopyBuffer(indexBuffer, fullCacheData, 0);
        io::CopyBuffer(mCache.mBuffer, fullCacheData, io::size_data(indexBuffer));

        io::SingleBLobLoader<io::BinAsset> cacheLoader(mVTS, mCacheSection);
        if (auto asset = cacheLoader.GetAsset())
        {
            asset->mBuffer = fullCacheData;
            mVTS.UpdateAssetData(asset, io::VirtualTransportSystem::Request::UpdateOnly);
        }
        else
        {
            auto tag = mVTS.GenerateTag(mCacheSection);
            std::shared_ptr<io::Asset> newAsset = io::ResolveAsset<io::BinAsset>(fullCacheData, tag, mVTS);
            mVTS.UpdateAssetData(newAsset, io::VirtualTransportSystem::Request::Add);
        }
    }
}


//-------------------------------------------------------------------------------------------------
yaget::io::Buffer defensor::render::ShaderCache::GetShader(const yaget::io::Tag& tag) const
{
    if (auto it = mCacheIndex.find(tag.mGuid); it != mCacheIndex.end())
    {
        // we need a way to just point Buffer into existing memory without creating a new buffer
        const auto& location = it->second;
        return io::CreateBuffer(io::cast_data<const char>(mCache.mBuffer) + location.mOffset, location.mSize);
    }                                                             
    return {};
}


void defensor::render::ShaderCache::SaveShader(const yaget::io::Tag& tag, yaget::io::Buffer buffer)
{
    // see if we already have this shader saved and if size matches, just overwrite
    if (auto it = mCacheIndex.find(tag.mGuid); it != mCacheIndex.end())
    {
        const auto& location = it->second;
        if (location.mSize == io::size_data(buffer))
        {
            io::CopyBuffer(buffer, mCache.mBuffer, location.mOffset);
            mCacheDirty = true;
            return;
        }

        std::memset(io::cast_data<char>(mCache.mBuffer) + location.mOffset, 0, location.mSize);
        mCacheIndex.erase(it);
    }

    mCacheIndex.insert({ tag.mGuid, { mCache.mWriteOffset, io::size_data(buffer) }});
    mCache.AssureWriteSize(io::size_data(buffer));
    mCache.WriteDataChunk(buffer);
    mCacheDirty = true;
}


//-------------------------------------------------------------------------------------------------
defensor::render::RenderShader::RenderShader(yaget::io::VirtualTransportSystem& vts)
    : mVTS(vts)
    , mShaderCache(mVTS, yaget::io::VirtualTransportSystem::Section("Caches@Shaders"))
{
}


//-------------------------------------------------------------------------------------------------
defensor::render::RenderShader::~RenderShader() = default;


//-------------------------------------------------------------------------------------------------
yaget::io::Buffer defensor::render::RenderShader::GetShader(const yaget::io::Tag& tag, ShaderType shaderType)
{
    YAGET_ASSERT(tag.IsValid(), "Tag: '%s:%s' is not valid.", yaget::conv::Convertor<yaget::Guid>::ToString(tag.mGuid).c_str(), yaget::conv::Convertor<yaget::io::Tag>::ToString(tag).c_str());

    if (auto it = mShaders.find(tag); it != mShaders.end())
    {
        return it->second;
    }

    if (auto shader = mShaderCache.GetShader(tag); yaget::io::size_data(shader))
    {
        mShaders.insert({ tag, shader });
        return shader;
    }

    const char* entryName = ShaderMappings[shaderType].mEntryPoint;
    const char* target = ShaderMappings[shaderType].mTarget;

    yaget::io::Buffer shaderBuffer;
    if (tag.mName == "EmbeddedVertexShader" || tag.mName == "EmbeddedPixelShader")
    {
        shaderBuffer = io::CreateBuffer(shaderSource, std::strlen(shaderSource));
    }
    else
    {
        io::SingleBLobLoader<io::StringsAsset> shaderLoader(mVTS, tag);
        if (auto asset = shaderLoader.GetAsset())
        {
            shaderBuffer = asset->mBuffer;
        }
    }
    
    yaget::io::Buffer result;
    if (io::size_data(shaderBuffer))
    {
        yaget::render::ResourceCompiler compiler(io::cast_to_view(shaderBuffer), entryName, target, false /*useNewestCompiler*/);
        result = compiler.GetCompiled();
        mShaders.insert({ tag, result });

        mShaderCache.SaveShader(tag, result);
    }
    else
    {
        YLOG_ERROR("REND", "There is no data associated with shader: '%s'.", conv::Convertor<io::Tag>::ToString(tag).c_str());
    }

    return result;
}


//-------------------------------------------------------------------------------------------------
std::vector<yaget::io::Buffer> defensor::render::RenderShader::GetShaders(const yaget::io::Tags& tags, ShaderType shaderType)
{
    std::vector<yaget::io::Buffer> results = 
        tags | 
        std::views::transform([this, shaderType](const auto& tag)
        {
            return GetShader(tag, shaderType);
        }) | 
        std::ranges::to<std::vector>();

    return results;
}
