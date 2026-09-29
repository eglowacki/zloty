#include "Render/RenderCore.h"
#include "Compression/Zipper.h"
#include "Render/Cache/AssetCache.h"
#include "Render/Cache/CacheWatcher.h"
#include "Streams/Guid.h"
#include "VTS/ResolvedAssets.h"


//-------------------------------------------------------------------------------------------------
namespace
{
    constexpr size_t CurrentFileVersion = 1;

    size_t SaveDataCollection(auto dataCollection, char* dataPointer, size_t offset)
    {
        using namespace yaget;

        auto numElements = dataCollection.size();
        offset = io::mem::WriteData(dataPointer, offset, numElements);
        
        for (const auto& [key, value] : dataCollection)
        {
            offset = io::mem::WriteData(dataPointer, offset, key);
            offset = io::mem::WriteData(dataPointer, offset, value);
        }

        return offset;
    }

    template <typename T>
    T LoadDataCollection(const char* dataPointer, size_t& offset)
    {
        using namespace yaget;
        using Key = typename T::key_type;
        using Value = typename T::mapped_type;

        T result{};

        auto numElements = *io::mem::ReadData<size_t>(dataPointer, offset);
        for (size_t i = 0; i < numElements; ++i)
        {
            auto key = io::mem::ReadData<Key>(dataPointer, offset);
            auto value = io::mem::ReadData<Value>(dataPointer, offset);

            result.insert({ *key, *value });
        }

        return result;
    }


    // Return true if cache is valid and up to date, otherwise false if cache is stale and needs to be resaved. 
    bool ValidateCacheIndex(auto& cacheIndex, const std::filesystem::file_time_type& cacheFileTimeStamp, const std::string& cacheFilePath, const yaget::io::VirtualTransportSystem& vts)
    {
        using namespace yaget;

        auto erasedElements = std::erase_if(cacheIndex, [&vts, &cacheFileTimeStamp, &cacheFilePath](const auto& pair)
        {
            const auto& guid = pair.first;
            auto tag = vts.FindTag(guid);
            const auto filePath = tag.ResolveVTS();
            const auto fileTimeStamp = io::file::GetFileDate(filePath);
            if (fileTimeStamp > cacheFileTimeStamp)
            {
                // skip getting cache data for this file since it's newer then cache file
                YLOG_INFO("DEVI", std::format("File '{} ({})' is newer then cache '{} ({})', will update.", filePath, fileTimeStamp, cacheFilePath, cacheFileTimeStamp).c_str());
                return true;
            }

            return false;
        });

        return erasedElements == 0;
    }

}


yaget::render::AssetCache::TagToAssetCacheTypeMap yaget::render::AssetCache::mTagToAssetCacheType = {};


//-------------------------------------------------------------------------------------------------
void yaget::render::AssetCache::PopulateMappings(const Section& /*fileName*/, io::VirtualTransportSystem& /*vts*/)
{
    //PopulateMap<TypeToSectionMap>(fileName, vts, TypeToSection);
}


//-------------------------------------------------------------------------------------------------
void yaget::render::AssetCache::SaveMappings(const Section& /*fileName*/, io::VirtualTransportSystem& /*vts*/)
{
    //SaveMap(fileName, vts, TypeToSection);
}


//-------------------------------------------------------------------------------------------------
yaget::render::AssetCache::AssetCache(io::VirtualTransportSystem& vts, Section fileName, io::Buffer userData)
    : mVTS(vts)
    , mCacheSection(std::move(fileName))
    , mUserData(std::move(userData))
{
    const auto& configBlock = dev::CurrentConfiguration().mDataLoaders;
    if (!configBlock.mClearCache)
    {
        const auto cacheFileTag = mVTS.GetTag(mCacheSection);
        const auto cacheFilePath = cacheFileTag.ResolveVTS();
        const auto cacheFileTimeStamp = io::file::GetFileDate(cacheFilePath);

#if 0
        const auto cacheFileTimeStamp2 = io::file::GetFileDate(cacheFilePath, io::file::FileDateType::CreationTime);
        cacheFileTimeStamp2;
#endif

        io::SingleBLobLoader<io::BinAsset> cacheLoader(mVTS, mCacheSection);
        if (auto asset = cacheLoader.GetAsset())
        {
            const size_t USER_DATA_SIZE = io::size_data(mUserData);
            io::MessagingBuffer cache;
            cache.mBuffer = compression::UnzipBuffer(io::cast_to_view(asset->mBuffer));
            if (!io::size_data(cache.mBuffer))
            {
                cache.mBuffer = asset->mBuffer;
            }
            cache.mWriteOffset = io::size_data(cache.mBuffer);

            auto dataPointer = io::cast_data<const char>(cache.mBuffer);
            size_t offset = 0;
                                                                       
            auto fileSignature = io::mem::ReadData<YagetFileSignature>(dataPointer, offset);
            if (!fileSignature->IsValid(CurrentFileVersion))
            {
                YLOG_ERROR("DEVI", "Unsupported cache '%s' version: '%d'. Expected version is <= '%d'. Cache will be ignored.",
                    conv::Convertor<Section>::ToString(mCacheSection).c_str(),
                    fileSignature->Version,
                    CurrentFileVersion);
                return;
            }

            // if file is NOT the same version as latest supported by the loader, we need to re-save it on dtor, so make it 'dirty'
            mCacheStatus = fileSignature->Version == CurrentFileVersion ? CacheStatus::Clean : CacheStatus::Dirty;
            if (fileSignature->Version == 1)
            {
                // if cache file user data is not the same provided signature, we need to re-save it on dtor, so make it 'dirty'
                if (std::memcmp(dataPointer + offset, io::cast_data<const char>(mUserData), USER_DATA_SIZE) != 0)
                {
                    YLOG_INFO("DEVI", "Outdated user data in cache '%s'. Cache will be ignored.", conv::ToString(mCacheSection).c_str());

                    mCacheStatus = CacheStatus::Dirty;
                    return;
                }
                offset += USER_DATA_SIZE;

                mCacheIndex = LoadDataCollection<decltype(mCacheIndex)>(dataPointer, offset);
                bool cacheValid = ValidateCacheIndex(mCacheIndex, cacheFileTimeStamp, cacheFilePath, mVTS);
                mCacheStatus = cacheValid ? mCacheStatus : ored(mCacheStatus, CacheStatus::Holes);

                mCache = io::MessagingBuffer(io::size_data(cache.mBuffer) - offset);
                mCache.mWriteOffset = io::size_data(mCache.mBuffer);
                memcpy(io::cast_data<char>(mCache.mBuffer), io::cast_data<char>(cache.mBuffer) + offset,
                    io::size_data(mCache.mBuffer));
            }
        }
    }
    else
    {
        // - do we just mark this as a dirty so it will get saved in dtor?
        // - do we want that option to be like fire and forget
    }
}


//-------------------------------------------------------------------------------------------------
yaget::render::AssetCache::~AssetCache()
{
    if (mCacheStatus != CacheStatus::Clean)
    {
        // we need to serialize mCacheIndex and mCache into a single buffer and save it back to VTS
        // format of the buffer is [YagetFileSignature][numElements][{guid}{location}...][cacheData]
        const size_t SIG_SIZE = sizeof(YagetFileSignature);
        const size_t USER_SIZE = io::size_data(mUserData);
        const size_t PAYLOAD_ENTRIES = sizeof(size_t);
        const size_t PAYLOAD_SIZE = (mCacheIndex.size() * (sizeof(Guid) + sizeof(Location)));
        io::Buffer indexBuffer = io::CreateBuffer(SIG_SIZE + USER_SIZE + PAYLOAD_ENTRIES + PAYLOAD_SIZE);

        YagetFileSignature fileSignature;
        fileSignature.Version = CurrentFileVersion;

        auto dataPointer = io::cast_data<char>(indexBuffer);
        size_t offset = 0;

        offset = io::mem::WriteData(dataPointer, offset, fileSignature);

        //-----------------------------------------------------------------------------
        // TODO(eg) write user data out. Q: How we determine at this level what is the user data and how we save it?
        std::memcpy(dataPointer + offset, io::cast_data<const char>(mUserData), USER_SIZE);
        offset += USER_SIZE;

        offset = SaveDataCollection(mCacheIndex, dataPointer, offset);

        mCache.Shrink();
        auto fullCacheData = io::CreateBuffer(io::size_data(indexBuffer) + io::size_data(mCache.mBuffer));
        io::CopyBuffer(indexBuffer, fullCacheData, 0);
        io::CopyBuffer(mCache.mBuffer, fullCacheData, io::size_data(indexBuffer));

        const auto& useZip = yaget::dev::CurrentConfiguration().mDataLoaders.mUseZip;

        auto compressedBuffer = useZip ? compression::ZipBuffer(io::cast_to_view(fullCacheData)) : io::Buffer{};
        io::Buffer* bufferToSave = io::size_data(compressedBuffer) ? &compressedBuffer : &fullCacheData;

        io::SingleBLobLoader<io::BinAsset> cacheLoader(mVTS, mCacheSection);
        if (auto asset = cacheLoader.GetAsset())
        {
            asset->mBuffer = *bufferToSave;
            mVTS.UpdateAssetData(asset, io::VirtualTransportSystem::Request::UpdateOnly);
        }
        else
        {
            auto tag = mVTS.GenerateTag(mCacheSection);
            std::shared_ptr<io::Asset> newAsset = io::ResolveAsset<io::BinAsset>(*bufferToSave, tag, mVTS);
            mVTS.UpdateAssetData(newAsset, io::VirtualTransportSystem::Request::Add);
        }
    }
}


//-------------------------------------------------------------------------------------------------
yaget::io::Buffer yaget::render::AssetCache::GetCachedAsset(const io::Tag& tag) const
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
void yaget::render::AssetCache::SaveCachedAsset(const io::Tag& tag, io::Buffer buffer)
{
    // see if we already have this shader saved and if size matches, just overwrite
    if (auto it = mCacheIndex.find(tag.mGuid); it != mCacheIndex.end())
    {
        auto& location = it->second;
        if (location.mSize == io::size_data(buffer))
        {
            // NOTE(eg) We need to still save smaller buffer, so we do not re-allocate
            // memory unnecessary. But we need to update
            // mCacheIndex[tag.mGuid] = mCache.mWriteOffset, io::size_data(buffer)};
            //location.mSize = io::size_data(buffer);
            io::CopyBuffer(buffer, mCache.mBuffer, location.mOffset);
            mCacheStatus = ored(mCacheStatus, CacheStatus::Dirty);

            return;
        }
        std::memset(io::cast_data<char>(mCache.mBuffer) + location.mOffset, 0, location.mSize);
        mCacheIndex.erase(it);
    }
    mCacheIndex.insert({ tag.mGuid, {mCache.mWriteOffset, io::size_data(buffer)} });
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


//-------------------------------------------------------------------------------------------------
yaget::render::AssetCacheType yaget::render::AssetCache::TagToType(const io::Tag& tag)
{
    if (auto it = mTagToAssetCacheType.find(tag); it != mTagToAssetCacheType.end())
    {
        return it->second;
    }

    return AssetCacheType::Empty;
}


//-------------------------------------------------------------------------------------------------
void yaget::render::AssetCache::AddTagToType(const io::Tag& tag, AssetCacheType assetCacheType)
{
    mTagToAssetCacheType[tag] = assetCacheType;
}
