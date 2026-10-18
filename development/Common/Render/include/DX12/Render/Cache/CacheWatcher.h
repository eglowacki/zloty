///////////////////////////////////////////////////////////////////////
// CacheWatcher.h
//
//  Copyright 01/19/2026 Edgar Glowacki.
//
//  Maintained by: Edgar
//
//  NOTES:
//
//  #include "Render/Cache/CacheWatcher.h"
//
//////////////////////////////////////////////////////////////////////
//! \file
#pragma once

#include "AssetCache.h"
#include "Streams/Buffers.h"
#include "VTS/ResolvedAssets.h"
#include "VTS/VirtualTransportSystem.h"


namespace yaget::render
{
    //-------------------------------------------------------------------------------------------------
    template <typename A>
    class CacheWatcher
    {
    public:
        CacheWatcher(io::VirtualTransportSystem& vts, io::VirtualTransportSystem::Section fileName, io::Buffer userData)
            : mVTS(vts)
            , mCache(mVTS, std::move(fileName), std::move(userData))
        {
        }
        ~CacheWatcher() = default;

        void ClearCache(const io::Tag& tag)
        {
            std::lock_guard mutexLocker(mMutex);

            mVTS.ClearAsset(tag);
            mCache.ClearCachedAsset(tag);
            mAssets.erase(tag);
        }

    protected:
        A GetAsset(const io::Tag& tag, std::function<A(const io::Tag& tag, io::Buffer& buffer)> assetCreation)
        {
            A result = GetAsset(tag);
            if (result == A{})
            {
                io::Buffer cachedData = mCache.GetCachedAsset(tag);
                bool missingCachedData = io::size_data(cachedData) == 0;

                result = assetCreation(tag, cachedData);

                if (result != A{})
                {
                    mAssets.insert({ tag, result });
                }

                if (io::size_data(cachedData) && missingCachedData)
                {
                    mCache.SaveCachedAsset(tag, cachedData);
                }
            }

            return result;
        }

        A GetAsset(const io::Tag& tag)
        {
            if (auto it = mAssets.find(tag); it != mAssets.end())
            {
                return it->second;
            }

            return {};
        }

        io::VirtualTransportSystem& mVTS;
        yaget::render::AssetCache mCache;
        std::map<io::Tag, A> mAssets;

        mutable std::mutex mMutex;
    };

}
