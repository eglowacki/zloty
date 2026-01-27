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
#include "Parsers/DependencyGraph.h"
#include "Streams/Buffers.h"
#include "Streams/Watcher.h"
#include "VTS/VirtualTransportSystem.h"


namespace yaget
{
    class DependencyGraph;
}

namespace yaget::render
{
    //-------------------------------------------------------------------------------------------------
    template <typename A>
    class CacheWatcher
    {
    public:
        CacheWatcher(io::VirtualTransportSystem& vts, io::VirtualTransportSystem::Section fileName, DependencyGraph& dependencyGraph)
            : mVTS(vts)
            , mCache(mVTS, fileName)
            , mDependencyGraph(dependencyGraph)
        {
        }

    protected:
        A GetAsset(const io::Tag& tag, std::function<A(const io::Tag& tag, io::Buffer& buffer)> assetCreation)
        {
            A result = GetAsset(tag);
            if (result == A{})
            {
                io::Buffer cachedData = mCache.GetCachedAsset(tag);
                bool missingCachedData = yaget::io::size_data(cachedData) == 0;

                result = assetCreation(tag, cachedData);

                mAssets.insert({tag, result});

                if (yaget::io::size_data(cachedData) && missingCachedData)
                {
                    mCache.SaveCachedAsset(tag, cachedData);
                }
            }

            return result;
        }

        A GetAsset(const io::Tag& tag)
        {
            AssureTagWatch(tag);

            if (auto it = mAssets.find(tag); it != mAssets.end())
            {
                return it->second;
            }

            return {};
        }

        void ClearTagWatch(const yaget::io::Tag& tag)
        {
            std::lock_guard mutexLocker(mMutex);

            if (mWatchedTags.contains(tag))
            {
                mWatchedTags.erase(tag);
                mWatcher.Remove(tag.Hash());
            }
        }

        io::VirtualTransportSystem& mVTS;
        yaget::render::AssetCache mCache;
        io::Watcher mWatcher;
        std::set<io::Tag> mWatchedTags;
        std::map<io::Tag, A> mAssets;

        std::mutex mMutex;

    private:
        // will add to watch file changes if the file is not already watched
        void AssureTagWatch(const yaget::io::Tag& tag)
        {
            if (auto node = mDependencyGraph.Find(tag.mGuid, nullptr); node->mDirty)
            {
                node->mDirty = false;
                mAssets.erase(tag);
                mCache.ClearCachedAsset(tag);
                mVTS.ClearAsset(tag);
            }

            if (!mWatchedTags.contains(tag))
            {
                if (auto shaderFilePath = tag.ResolveVTS(); !shaderFilePath.empty())
                {
                    mWatchedTags.insert(tag);

                    mWatcher.Add(tag.Hash(), shaderFilePath, [this, tag]()
                    {
                        std::lock_guard mutexLocker(mMutex);

                        std::vector<yaget::DependencyNode*> pathTo;
                        if (yaget::DependencyNode* shaderNode = mDependencyGraph.Find(tag.mGuid, &pathTo))
                        {
                            std::for_each(pathTo.begin(), pathTo.end(), [](auto& node)
                            {
                                node->mDirty = true;
                            });
                        }
                    });
                }
            }
        }

        DependencyGraph& mDependencyGraph;
    };

}
