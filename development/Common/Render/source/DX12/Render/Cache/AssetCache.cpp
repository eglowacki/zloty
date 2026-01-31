#include "Render/Cache/AssetCache.h"
#include "Render/Platform/ResourceCompiler.h"
#include <VertexTypes.h>

#include "HashUtilities.h"
#include "Streams/Guid.h"
#include "VTS/ResolvedAssets.h"


//-------------------------------------------------------------------------------------------------
namespace
{
    std::string CacheTypeToString(yaget::render::AssetCacheType value) 
    {
        std::string result;

        auto temp_value = static_cast<uint64_t>(value); // Work with a copy
        
        while (temp_value != 0) 
        {
            // The expression (temp_value & -temp_value) isolates the least significant set bit
            uint64_t lsb = temp_value & (-static_cast<int64_t>(temp_value)); 
            
            auto singleValue = static_cast<yaget::render::AssetCacheType>(lsb);
            const auto enumName = magic_enum::enum_name(singleValue);

            result += result.empty() ? enumName : " | " + std::string(enumName);

            // Clear the least significant set bit
            temp_value ^= lsb; 
            // Alternatively, a common trick to clear the LSB: temp_value &= (temp_value - 1);
        }

        return result;
    }
    
}


//-------------------------------------------------------------------------------------------------
namespace yaget::render
{
    inline void to_json(nlohmann::json& j, const AssetCacheType cacheType)
    {
        const auto enumName = CacheTypeToString(cacheType);
        j = enumName;
    }

    inline void from_json(const nlohmann::json& j, AssetCacheType& cacheType)
    {
        std::string source;
        j.get_to(source);

        AssetCacheType result{};
        auto bitValues = conv::Split(source, "|", true);
        for (auto bit : bitValues)
        {
            auto enumBit = magic_enum::enum_cast<AssetCacheType>(bit);
            if (enumBit.has_value())
            {
                result = result | enumBit.value();
            }
        }

        cacheType = result;
    }
}

//-------------------------------------------------------------------------------------------------
yaget::render::AssetCache::TypeToSectionMap yaget::render::AssetCache::TypeToSection =
{
    {BasicVertex, yaget::io::VirtualTransportSystem::Section("VeretexShaders@Basic")},
    {BasicPixel, yaget::io::VirtualTransportSystem::Section("PixelShaders@Basic")},
    {BasicSignature, yaget::io::VirtualTransportSystem::Section("Transient@BasicSig")},
    {BasicPipeline, yaget::io::VirtualTransportSystem::Section("Transient@BasicPipe")},
};


//-------------------------------------------------------------------------------------------------
yaget::io::VirtualTransportSystem::Section yaget::render::AssetCache::operator[](AssetCacheType typeFlag)
{
    if (auto it = TypeToSection.find(typeFlag); it != TypeToSection.end())
    {
        return it->second;
    }
    YLOG_ERROR("DEVI", "There is no asset section associated with AssetCacheType: '%d'", typeFlag);
    return {};
}


//-------------------------------------------------------------------------------------------------
void yaget::render::AssetCache::PopulateTypeToSection(io::VirtualTransportSystem::Section fileName, io::VirtualTransportSystem& vts)
{
    TypeToSectionMap newMap = io::LoadBlob<TypeToSectionMap>(vts, fileName);
    if (!newMap.empty() && newMap != TypeToSection)
    {
        TypeToSection = newMap;
    }
}


//-------------------------------------------------------------------------------------------------
void yaget::render::AssetCache::SaveTypeToSection(io::VirtualTransportSystem::Section fileName, io::VirtualTransportSystem& vts)
{
    nlohmann::json jsonBlock = TypeToSection;
    auto textBlock = json::PrettyPrint(jsonBlock);
    io::Buffer buffer = io::CreateBuffer(textBlock);
    if (auto saveFileTag = vts.GetTag(fileName); saveFileTag.IsValid())
    {
        auto oldMap = io::LoadBlob<TypeToSectionMap>(vts, saveFileTag);
        if (oldMap.empty() || oldMap != TypeToSection)
        {
            io::SingleBLobLoader<io::JsonAsset> cacheLoader(vts, saveFileTag);
            auto asset = cacheLoader.GetAsset();
            asset->mBuffer = buffer;
            vts.UpdateAssetData(asset, io::VirtualTransportSystem::Request::UpdateOnly);
        }

    }
    else
    {
        auto newTag = vts.GenerateTag(fileName);
        std::shared_ptr<io::Asset> newAsset = io::ResolveAsset<io::JsonAsset>(buffer, newTag, vts);
        vts.UpdateAssetData(newAsset, io::VirtualTransportSystem::Request::Add);
    }
}


//-------------------------------------------------------------------------------------------------
yaget::render::AssetCache::AssetCache(io::VirtualTransportSystem& vts, io::VirtualTransportSystem::Section fileName)
    : mVTS(vts)
    , mCacheSection(fileName)
{
    const auto& configBlock = dev::CurrentConfiguration().mGraphics;
    if (!configBlock.mClearCache)
    {
        io::SingleBLobLoader<io::BinAsset> cacheLoader(mVTS, mCacheSection);
        if (auto asset = cacheLoader.GetAsset())
        {
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
                mCacheIndex.insert({*guid, *location});
            }
            mCache = io::MessagingBuffer(io::size_data(cache.mBuffer) - offset);
            mCache.mWriteOffset = io::size_data(mCache.mBuffer);
            memcpy(io::cast_data<char>(mCache.mBuffer), io::cast_data<char>(cache.mBuffer) + offset,
                   io::size_data(mCache.mBuffer));
        }
    }
}


//-------------------------------------------------------------------------------------------------
yaget::render::AssetCache::~AssetCache()
{
    if (mCacheStatus != CacheStatus::Clean)
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
yaget::io::Buffer yaget::render::AssetCache::GetCachedAsset(const yaget::io::Tag& tag) const
{
    if (auto it = mCacheIndex.find(tag.mGuid); it != mCacheIndex.end())
    {
        // we need a way to just point Buffer into existing memory without creating a new buffer
        const auto& location = it->second;
        return io::CreateBuffer(io::cast_data<const char>(mCache.mBuffer) + location.mOffset, location.mSize);
    }
    return {};
}


//-------------------------------------------------------------------------------------------------
void yaget::render::AssetCache::SaveCachedAsset(const yaget::io::Tag& tag, yaget::io::Buffer buffer)
{
    // see if we already have this shader saved and if size matches, just overwrite
    if (auto it = mCacheIndex.find(tag.mGuid); it != mCacheIndex.end())
    {
        const auto& location = it->second;
        if (location.mSize == io::size_data(buffer))
        {
            io::CopyBuffer(buffer, mCache.mBuffer, location.mOffset);
            mCacheStatus = ored(mCacheStatus, CacheStatus::Dirty);
            return;
        }
        std::memset(io::cast_data<char>(mCache.mBuffer) + location.mOffset, 0, location.mSize);
        mCacheIndex.erase(it);
    }
    mCacheIndex.insert({tag.mGuid, {mCache.mWriteOffset, io::size_data(buffer)}});
    mCache.AssureWriteSize(io::size_data(buffer));
    mCache.WriteDataChunk(buffer);
    mCacheStatus = ored(mCacheStatus, CacheStatus::Holes);
}


//-------------------------------------------------------------------------------------------------
void yaget::render::AssetCache::ClearCachedAsset(const io::Tag& tag)
{
    mCacheIndex.erase(tag.mGuid);
    mCacheStatus = ored(mCacheStatus, CacheStatus::Dirty);
}
