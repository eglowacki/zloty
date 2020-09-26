//////////////////////////////////////////////////////////////////////
// ResourceWatchRefresher.h
//
//  Copyright 8/18/2019 Edgar Glowacki
//
//  Maintained by: Edgar
//
//  NOTES:
//      Provides helper class to manage, and update render Resources when changed on disk/storage
//
//
//  #include "Resources/ResourceWatchRefresher.h"
//
//////////////////////////////////////////////////////////////////////
//! \file
#pragma once

#include "YagetCore.h"
#include "ThreadModel/Variables.h"
#include "Streams/Buffers.h"

namespace yaget
{
    namespace render
    {
        class Device;

        // Watcher can run in watched and non-watched mode
        struct UpdatePolicyWatched
        {
            static uint64_t GetWatchId(Device& device);
        };

        struct UpdatePolicyNonWatched
        {
            static uint64_t GetWatchId(Device& /*device*/) { return 0; }
        };

#ifdef YAGET_SHIPPING
        using DeafultUpdatePolicy = UpdatePolicyNonWatched;
#else
        using DeafultUpdatePolicy = UpdatePolicyWatched;
#endif // YAGET_SHIPPING


        // ------------------------------------------------------------------------------------------------------------
        template <typename T, typename Policy>
        struct ResourceWatchRefresher : public Noncopyable<ResourceWatchRefresher<T, Policy>>
        {
            using SmartType = typename mt::SmartVariable<T>::SmartType;

            ResourceWatchRefresher(Device& device, const io::Tag& tag);
            ~ResourceWatchRefresher();

            Device& mDevice;
            uint64_t mRefreshId = 0;
            mt::SmartVariable<T> mTrackedResource;
        };


        template <typename T, typename Policy>
        ResourceWatchRefresher<T, Policy>::ResourceWatchRefresher(Device& device, const  io::Tag& tag)
            : mDevice(device)
            , mRefreshId(Policy::GetWatchId(mDevice))
        {
            mDevice.RequestResourceView<T>(tag, std::ref(mTrackedResource), mRefreshId);
        }

        template <typename T, typename Policy>
        ResourceWatchRefresher<T, Policy>::~ResourceWatchRefresher()
        {
            mDevice.RemoveWatch(mRefreshId);
        }
    
        // ------------------------------------------------------------------------------------------------------------
        template <typename T, typename Policy = DeafultUpdatePolicy>
        class ResourceWatchCollection : public Noncopyable<ResourceWatchCollection<T, Policy>>
        {
        public:
            using ResourceWatch = ResourceWatchRefresher<T, Policy>;

            ResourceWatchCollection(Device& device, const io::Tags& tags);

            using ProcessCallback = std::function<bool(typename ResourceWatch::SmartType resurce)>;
            bool Process(const std::string& name, ProcessCallback callback);
            bool Process(ProcessCallback callback);

        private:
            Device& mDevice;
            using ResourceValue = std::unique_ptr<ResourceWatch>;
            using Resources = std::unordered_map<std::string, ResourceValue>;
            Resources mResources;
        };


        template <typename T, typename Policy>
        ResourceWatchCollection<T, Policy>::ResourceWatchCollection(Device& device, const io::Tags& tags)
            : mDevice(device)
        {
            for (const auto& tag : tags)
            {
                mResources.insert(std::make_pair(tag.mName, std::make_unique<ResourceWatch>(mDevice, tag)));
            }
        }


        template <typename T, typename Policy>
        bool ResourceWatchCollection<T, Policy>::Process(const std::string& name, ProcessCallback callback)
        {
            if (auto it = mResources.find(name); it != mResources.end())
            {
                typename ResourceWatch::SmartType trackedResource = it->second->mTrackedResource;
                if (trackedResource)
                {
                    return callback(trackedResource);
                }
            }

            return false;
        }


        template <typename T, typename Policy>
        bool ResourceWatchCollection<T, Policy>::Process(ProcessCallback callback)
        {
            std::size_t numResources = mResources.size();
            for (auto& watched: mResources)
            {
                typename ResourceWatch::SmartType trackedResource = watched.second->mTrackedResource;
                if (trackedResource)
                {
                    if (!callback(trackedResource))
                    {
                        return false;
                    }

                    --numResources;
                }
            }

            return !mResources.empty() && numResources == 0;
        }
    } // namespace render
} // namespace yaget
