#include "Render/Cache/AssetCache.h"
#include "Render/Platform/ResourceCompiler.h"
#include <VertexTypes.h>
#include "Streams/Guid.h"
#include "VTS/ResolvedAssets.h"

//------------------------------------------------------------------------------------------------- 
std::map<uint64_t, yaget::io::Tag> yaget::render::AssetCache::TypeToTag =
{
    {1, {"One", yaget::Guid("0B1E1308-FA78-4341-896A-B12BF1F0BA43") }},
    {2, {"Two", yaget::Guid("FA1E875D-277B-4C1D-A31E-31EF6ADD6253") }},
    {3, {"Three", yaget::Guid("96E3870E-CE8F-472D-AC1B-C4D89B61F893") }},
    {4, {"Four", yaget::Guid("C6A31CA5-2261-4745-88DB-FC9CE460BBE5") }},
    {5, {"Five", yaget::Guid("FAA49895-41DD-4FF0-880A-83DB868311D4") }},
    {6, {"Six", yaget::Guid("066ECD15-CE86-4FF6-8193-B56FDDD5E874") }},
    {7, {"Seven", yaget::Guid("FA8DE50F-D05A-48EE-A3B0-2D8D5778F352") }},
    {8, {"Eight", yaget::Guid("0B403C34-A121-416C-A334-E25A749A9B74") }},
    {9, {"Nine", yaget::Guid("EA2E941C-A738-4AFB-BC8B-5BF6827A7666") }},
    {10, {"Ten", yaget::Guid("06BD9DF5-8435-4DBD-BB9A-E938CD72E914") }},
    {11, {"Eleven", yaget::Guid("2F7F5788-FD58-4AA2-86EF-ACD2719B2E02") }},
    {12, {"Twelve", yaget::Guid("7A5D7803-3CA7-4B4A-9D3B-2F1DAE46F3D3") }},
    {13, {"Thirteen", yaget::Guid("CF540FA7-614E-4BEE-A5FD-A83B18272B75") }},
    {14, {"Fourteen", yaget::Guid("081E407C-FDBE-4AE2-9BC6-757F1CFAE55D") }},
    {15, {"Fifteen", yaget::Guid("ACADEE94-250B-422A-A566-98A0C4C0BBB4") }},
    {16, {"Sixteen", yaget::Guid("D3C5E3D1-1F2C-4C1E-8E2F-8D6F4B8C9A2E") }},
    {17, {"Seventeen", yaget::Guid("60190975-B0C2-414C-9C29-7155285AF5A9") }},
    {15, {"Eighteen", yaget::Guid("A521B9BB-194F-46B5-BAA0-A649B903C946") }},
    {16, {"Nineteen", yaget::Guid("68E1FD3D-B8BB-47AD-B939-259A96938A67") }},
    {17, {"Twenty", yaget::Guid("DF9F4AC1-4200-4CE8-9F29-BE17171B9274") }},
};


//-------------------------------------------------------------------------------------------------
yaget::render::AssetCache::AssetCache(io::VirtualTransportSystem& vts, io::VirtualTransportSystem::Section fileName)
    : mVTS(vts)
    , mCacheSection(fileName)
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
            mCacheIndex.insert({ *guid, *location });
        }

        mCache = io::MessagingBuffer(io::size_data(cache.mBuffer) - offset);
        mCache.mWriteOffset = io::size_data(mCache.mBuffer);
        memcpy(io::cast_data<char>(mCache.mBuffer), io::cast_data<char>(cache.mBuffer) + offset, io::size_data(mCache.mBuffer));
    }
}


//-------------------------------------------------------------------------------------------------
yaget::render::AssetCache::~AssetCache()
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


void yaget::render::AssetCache::SaveCachedAsset(const yaget::io::Tag& tag, yaget::io::Buffer buffer)
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
