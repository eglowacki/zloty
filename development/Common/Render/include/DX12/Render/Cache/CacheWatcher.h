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
#include "VTS/ResolvedAssets.h"
#include "VTS/VirtualTransportSystem.h"


namespace yaget::render
{
    template<typename T>
    void PopulateMap(io::VirtualTransportSystem::Section fileName, io::VirtualTransportSystem& vts, T& currentMap)
    {
        T newMap = yaget::io::LoadBlob<T>(vts, fileName);
        if (!newMap.empty() && newMap != currentMap)
        {
            currentMap = newMap;
        }
    }

    template<typename T>
    void SaveMap(io::VirtualTransportSystem::Section fileName, io::VirtualTransportSystem& vts, T& currentMap)
    {
        using namespace yaget;

        nlohmann::json jsonBlock = currentMap;
        auto textBlock = json::PrettyPrint(jsonBlock);
        io::Buffer buffer = io::CreateBuffer(textBlock);
        if (auto saveFileTag = vts.GetTag(fileName); saveFileTag.IsValid())
        {
            auto oldMap = io::LoadBlob<T>(vts, saveFileTag);
            if (oldMap.empty() || oldMap != currentMap)
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
    template <typename A>
    class CacheWatcher
    {
    public:
        CacheWatcher(io::VirtualTransportSystem& vts, io::VirtualTransportSystem::Section fileName, DependencyGraph& dependencyGraph, io::Watcher& watcher)
            : mVTS(vts)
            , mCache(mVTS, fileName)
            , mDependencyGraph(dependencyGraph)
            , mWatcher(watcher)
        {
        }
        ~CacheWatcher()
        {
            std::ranges::for_each(mWatchedTags, [this](const auto& tag)
            {
                mWatcher.Remove(tag.Hash());
            });
        }

        bool IsAsset(const io::Tag& tag) const
        {
            std::lock_guard mutexLocker(mMutex);
            return mAssets.contains(tag);
        }

        bool IsCached(const io::Tag& tag) const
        {
            std::lock_guard mutexLocker(mMutex);
            return mCache.IsCachedAsset(tag);
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
        io::Watcher& mWatcher;
        std::set<io::Tag> mWatchedTags;
        std::map<io::Tag, A> mAssets;

        mutable std::mutex mMutex;

    private:
        // will add to watch file changes if the file is not already watched
        void AssureTagWatch(const yaget::io::Tag& tag)
        {
            if (auto node = mDependencyGraph.Find(tag.mGuid, nullptr); node && node->mDirty)
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
