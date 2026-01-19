///////////////////////////////////////////////////////////////////////
// AssetCache.h
//
//  Copyright 01/18/2026 Edgar Glowacki.
//
//  Maintained by: Edgar
//
//  NOTES:
//
//  #include "Renders/Cache/AssetCache.h"
//
//////////////////////////////////////////////////////////////////////
//! \file
#pragma once

#include "Streams/Buffers.h"
#include "VTS/VirtualTransportSystem.h"


namespace yaget::render
{
    //-------------------------------------------------------------------------------------------------
    class AssetCache
    {
    public:
        AssetCache(io::VirtualTransportSystem& vts, io::VirtualTransportSystem::Section fileName);
        ~AssetCache();

        io::Buffer GetCachedAsset(const io::Tag& tag) const;
        void SaveCachedAsset(const io::Tag& tag, io::Buffer buffer);

        // used to map Pipeline and RootSignature types to asset tags
        static std::map<uint64_t, yaget::io::Tag> TypeToTag;

    private:
        io::VirtualTransportSystem& mVTS;
        size_t mCacheHash{};

        // map of where the blob of dats is located (.second - offset)
        struct Location
        {
            size_t mOffset{};
            size_t mSize{};
        };
        std::map<Guid, Location> mCacheIndex;
        io::MessagingBuffer mCache;
        bool mCacheDirty = false;

        io::VirtualTransportSystem::Section mCacheSection;
    };

}
