//////////////////////////////////////////////////////////////////////
// VirtualTransportSystem.h
//
//  Copyright 3/18/2018 Edgar Glowacki
//
//  Maintained by: Edgar
//
//  NOTES:
//      Provides virtual access to assets and how to load them asynced
//
//
//  #include "VTS/VirtualTransportSystem.h"
//
//////////////////////////////////////////////////////////////////////
//! \file

#pragma once

#include "Database/Database.h"
#include "Debugging/DevConfiguration.h"
#include "Json/JsonHelpers.h"
#include "Platform/Support.h"
#include "Streams/Buffers.h"
#include "VTS/BlobLoader.h"


namespace
{
    class SectionEntriesCollector;
}

namespace yaget
{
    namespace io
    {
        struct Tag;
        class VirtualTransportSystem;

        //-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
        //! Base class for all assets managed by VirtualTransportSystem class. It uses shared_ptr<Asset>.
        //! Derive new Asset class from this with whatever data it's needed.
        //! Then create AssetResolver functor to convert file data into specific runtime data.
        //! To get specific asset at runtime, call RequestBlob with callback to receive said asset when it's loaded and processed.
        //! Assets are stored internally by VirtualTransportSystem and are returned on subsequent request.
        //! Using templatize version RequestBlob, allows handling of proper casting above our code, adding some semblance of correctness
        class Asset : public Noncopyable<Asset>
        {
        public:
            virtual ~Asset() {}

            const io::Tag mTag;
            io::Buffer mBuffer;

            bool IsValid() const { return mValid; }
            bool operator <(const Asset& rhs) const { return mTag < rhs.mTag; }

        protected:
            Asset(const io::Tag& tag, io::Buffer buffer, const VirtualTransportSystem& vts) : mTag(tag), mBuffer(buffer), mVTS(vts) {}

            bool mValid = true;
            const VirtualTransportSystem& mVTS;
        };


        //! Fully multi-threaded and async requests and notifications for file data from "some" source
        class VirtualTransportSystem : public Noncopyable<VirtualTransportSystem>
        {
        public:
            //! Default    - default values
            //! Optimum    - optimized, used in runtime game
            //! Diagnostic - report diagnostic health of vts data
            //! Fix        - fix any vts data issues 
            //! Reset      - reset vts data and initialize as new 
            enum class RuntimeMode { Default, Optimum, Diagnostic, Fix, Reset };

            //--------------------------------------------------------------------------------------------------
            // Represents section's name and filter/file_name
            struct Section
            {
                //! How do we match filter into VTS Name
                // Prefix for Section is:
                //   Name as is - like match, mostly used to wild cards suffix, useful in file extensions
                // = Exact      - match exactly 
                // > Override   - find in latest Path entry (last path entry wins)
                enum class FilterMatch { Like, Exact, Override };
                const char FilterMatchCh[3][2]{ "", "=", ">" };

                Section();
                Section(const Section& source);
                Section(const io::Tag& tag);
                Section& operator=(const Section& source);
                bool operator==(const Section& other) const;

                //! Initializes with string in a format Name@Filter. Filter is optional.
                Section(const std::string& pathName);

                std::string ToString() const;

            private:
                std::string mName;       // Section name
                std::string mFilter;     // Filter of section. If empty, then grab all under section name

                FilterMatch mMatch = FilterMatch::Like;

            public:
                const std::string& Name;
                const std::string& Filter;
                const FilterMatch& Match;

            };
            using Sections = std::vector<Section>;

            // basic functor types for callbacks
            using DoneCallback = std::function<void()>;
            using BlobAssetCallback = std::function<void(std::shared_ptr<io::Asset> /*asset*/)>;
            using AssetResolver = std::function<std::shared_ptr<io::Asset>(const io::Buffer& /*dataBuffer*/, const io::Tag& /*requestedTag*/, VirtualTransportSystem& /*vts*/)>;
            using AssetResolvers = std::map<std::string, AssetResolver>;

            // find asset based on sectionName/blobName and call blobAssetCallback functor. blobAssetCallback might be called from different thread.
            size_t RequestBlob(const Section& section, BlobAssetCallback blobAssetCallback, std::atomic_size_t* tagsCounter) { return RequestBlob(Sections{ section }, blobAssetCallback, tagsCounter); }
            size_t RequestBlob(const Sections& sections, BlobAssetCallback blobAssetCallback, std::atomic_size_t* tagsCounter) { return RequestBlob(GetTags(sections), blobAssetCallback, tagsCounter); }
            size_t RequestBlob(const io::Tag& tag, BlobAssetCallback blobAssetCallback, std::atomic_size_t* tagsCounter) { return RequestBlob(std::vector<io::Tag>{ tag }, blobAssetCallback, tagsCounter); }
            size_t RequestBlob(const std::vector<io::Tag>& tags, BlobAssetCallback blobAssetCallback, std::atomic_size_t* tagsCounter);

            template<typename A>
            size_t RequestBlob(const Section& section, std::function<void(std::shared_ptr<A>)> blobAssetCallback, std::atomic_size_t* tagsCounter);
            template<typename A>
            size_t RequestBlob(const Sections& sections, std::function<void(std::shared_ptr<A>)> blobAssetCallback, std::atomic_size_t* tagsCounter);

            template<typename A>
            size_t RequestBlob(const io::Tag& tag, std::function<void(std::shared_ptr<A>)> blobAssetCallback, std::atomic_size_t* tagsCounter);
            template<typename A>
            size_t RequestBlob(const std::vector<io::Tag>& tags, std::function<void(std::shared_ptr<A>)> blobAssetCallback, std::atomic_size_t* tagsCounter);

            //! Return collection of all tags under sectionName/blobName
            std::vector<io::Tag> GetTags(const Section& section) const { return GetTags(Sections{ section }); }
            std::vector<io::Tag> GetTags(const Sections& sections) const;
            io::Tag GetTag(const Section& section) const { std::vector<io::Tag> tags = GetTags(section); return tags.empty() ? io::Tag() : *tags.begin(); }

            //! Return number of valid tags under sectionName/blobName
            size_t GetNumTags(const Section& section) const { return GetNumTags(Sections{ section }); }
            size_t GetNumTags(const Sections& sections) const;

            // Create tag data based on section. This includes new guid or recovered guid
            io::Tag GenerateTag(const Section& section) const;
            io::Tag AssureTag(const Section& section);

            //! Return true if section(s) does exist in DB 
            bool IsSectionValid(const Section& section) const { return IsSectionValid(Sections{ section }); }
            bool IsSectionValid(const Sections& sections) const;

            // Allows to provide already VTS asset, but wit local ones. It is not saved.
            void AddOverride(std::shared_ptr<io::Asset> asset);
            // Remove local cached assets, but preserve entry in DB. Used to force reload from disk on next request/load blob
            void ClearAssets(const io::Tags& tags);

            void AttachTransientBlob(const std::shared_ptr<io::Asset>& asset) { AttachTransientBlob({ asset }); }
            void AttachTransientBlob(const std::vector<std::shared_ptr<io::Asset>>& assets);

            enum class Request { UpdateOnly, Add };
            bool UpdateAssetData(const std::shared_ptr<io::Asset>& asset, Request request);

            // Return name of the resolver used for this specific asset tag
            std::string GetResolverType(const io::Tag& tag) const;

            virtual ~VirtualTransportSystem();

        protected:
            using VTSConfigList = dev::Configuration::Init::VTSConfigList;

            VirtualTransportSystem(VTSConfigList configList, DoneCallback doneCallback, const AssetResolvers& assetResolvers, const std::string& fileName, RuntimeMode reset);
            VirtualTransportSystem(RuntimeMode runtimeMode, const std::string& fileName);

            std::shared_ptr<Asset> AddAsset(std::shared_ptr<Asset> asset);
            void RemoveAsset(const io::Tag& tag);
            std::shared_ptr<Asset> FindAsset(const io::Tag& tag) const;

            //--------------------------------------------------------------------------------------------------
            // provides locking for DB for read/write, use LockDatabaseAccess() accessors to aquire one
            struct DatabaseLocker
            {
                DatabaseLocker(VirtualTransportSystem& vts) : mDatabase(vts.mDatabase) {}
                virtual ~DatabaseLocker() {}

                const SQLite& DB() const { return mDatabase.DB(); }
                SQLite& DB() { return mDatabase.DB(); }

            private:
                Database& mDatabase;
            };
            using DatabaseHandle = std::unique_ptr<DatabaseLocker>;

            virtual DatabaseHandle LockDatabaseAccess() { return std::make_unique<DatabaseLocker>(*this); }
            virtual DatabaseHandle LockDatabaseAccess() const { return std::make_unique<DatabaseLocker>(const_cast<VirtualTransportSystem&>(*this)); }

            const RuntimeMode mRuntimeMode;

        private:
            void onBlobLoaded(const io::Buffer& dataBuffer, const io::Tag& requestedTag, BlobAssetCallback assetLoaded, std::atomic_size_t* tagsCounter);
            void onErrorBlobLoader(const std::string& filePathName, const std::string& errorMessage);
            void onEntriesCollected();
            std::shared_ptr<Asset> FindAssetNonMT(const io::Tag& tag) const;
            std::shared_ptr<Asset> AddAssetNonMT(std::shared_ptr<Asset> asset);

            DoneCallback mDoneCallback;

            yaget::mt::JobPool mRequestPool;        // used to trigger callback for preloaded asset
            const AssetResolvers mAssetResolvers;   // callbacks to parse incoming blob data into specific asset
            mutable std::mutex mMutexAssets;        // control write/read to preloaded assets
            std::map<io::Tag, std::shared_ptr<Asset>> mAssets;
            std::map<io::Tag, std::shared_ptr<Asset>> mCashedAssets;
            Database mDatabase;                     // source of trues
            std::shared_ptr<SectionEntriesCollector> mSectionEntriesCollector;  // only used in gathering blobs on the disk and matching with db.
            BlobLoader mBlobLoader;                 // make sure that is always last in class here 
        };


        //--------------------------------------------------------------------------------------------------
        inline std::ostream& operator<<(std::ostream& out, const VirtualTransportSystem::Section& section)
        {
            return out << section.ToString();
        }

        inline void to_json(nlohmann::json& j, const VirtualTransportSystem::Section& section)
        {
            j = section.ToString();
        }

        inline void from_json(const nlohmann::json& j, VirtualTransportSystem::Section& section)
        {
            std::string source;
            j.get_to(source);

            section = VirtualTransportSystem::Section(source);
        }

        //--------------------------------------------------------------------------------------------------
        namespace db
        {
            // syntactic sugar for creation of db query command strings in uniform matter
            inline std::string TagRecordQuery(const VirtualTransportSystem::Section& section, const char* columns = nullptr)
            {
                using FilterMatch = VirtualTransportSystem::Section::FilterMatch;

                std::string columnsText = columns ? columns : "Guid, Name, VTS, Section";
                std::string frontWild = "%", endWild = "%";
                if (section.Match == FilterMatch::Override || section.Match == FilterMatch::Exact)
                {
                    frontWild = "";
                    endWild = ".%";
                }

                std::string vtsText = section.Filter.empty() ? ";" : fmt::format(" AND VTS LIKE '{}{}{}' ORDER BY VTS;", frontWild, section.Filter, endWild);
                std::string command = fmt::format("SELECT {} FROM Tags WHERE Section = '{}'{}", columnsText, section.Name, vtsText);
                return command;
            }

        } // namespace db


        //--------------------------------------------------------------------------------------------------
        //! Helper function to cast one type of asset smart pointer into another (A = T), it uses dynamic cast in debug, otherwise static
        template <typename A, typename T>
        inline std::shared_ptr<A> asset_cast(std::shared_ptr<T> asset)
        {
#ifdef YAGET_DEBUG
            std::shared_ptr<A> castAsset = std::dynamic_pointer_cast<A>(asset);
            YAGET_ASSERT(castAsset, "Could not cast Asset: '%s' from: '%s' to: '%s'.", (asset ? asset->mTag.ResolveVTS().c_str() : "NULL"), typeid(T).name(), typeid(A).name());
#else
            std::shared_ptr<A> castAsset = std::static_pointer_cast<A>(asset);
#endif // YAGET_DEBUG

            return castAsset;
        }


        //--------------------------------------------------------------------------------------------------
        // Synced call to load and get all assets,
        template <typename T>
        class BLobLoader : public Noncopyable<BLobLoader<T>>
        {
        public:
            using AssetPtr = std::shared_ptr<T>;
            using Collection = std::vector<AssetPtr>;
            using Section = io::VirtualTransportSystem::Section;
            using Sections = io::VirtualTransportSystem::Sections;
            using AssetCallback = io::VirtualTransportSystem::BlobAssetCallback;
            using DoneCallback = std::function<void(const Collection& collection)>;

            // will block until all tag(s) are loaded
            BLobLoader(io::VirtualTransportSystem& vts, const io::Tags& tags)
                : mVTS(vts)
            {
                mVTS.RequestBlob<T>(tags, [this](auto&&... params) { onBlobLoaded(params...); }, &mCountFiles);
                platform::Sleep([this]() { return mCountFiles != 0; });

                if (!mAssets.empty())
                {
                    for (auto tag : tags)
                    {
                        mList.push_back(mAssets[tag.mGuid]);
                    }
                }
            }

            BLobLoader(io::VirtualTransportSystem& vts, const io::Tag& tag)
                : BLobLoader(vts, io::Tags{ tag })
            {}

            // will block until all section(s) are loaded
            BLobLoader(io::VirtualTransportSystem& vts, const Sections& sections)
                : BLobLoader(vts, vts.GetTags(sections))
            {}

            BLobLoader(io::VirtualTransportSystem& vts, const Section& section)
                : BLobLoader(vts, Sections{ section })
            {}

            // will contain all assets loaded
            const Collection& Assets() const { return mList; }

        protected:
            io::VirtualTransportSystem& mVTS;

        private:
            using AssetMap = std::map<Guid, AssetPtr>;

            void onBlobLoaded(AssetPtr asset)
            {
                mAssets.insert(std::make_pair(asset->mTag.mGuid, asset));
            }

            std::atomic_size_t mCountFiles{ 0 };
            AssetMap mAssets;
            Collection mList;
            DoneCallback mDoneCallback;
        };

        template <typename T>
        class SingleBLobLoader : public BLobLoader<T>
        {
        public:
            SingleBLobLoader(io::VirtualTransportSystem& vts, const io::Tag& tag) : BLobLoader<T>(vts, tag)
            {}

            SingleBLobLoader(io::VirtualTransportSystem& vts, const BLobLoader<T>::Section& section) : BLobLoader<T>(vts, section)
            {}

            BLobLoader<T>::AssetPtr GetAsset() const
            {
                const auto& collection = BLobLoader<T>::Assets();
                return collection.empty() ? nullptr : *collection.begin();
            }

            struct NullAsset {};
            struct DefaultAsset {};

            // Get asset, but converted to F type. ReturnedAsset governs if we create instance of asset type F (DefaultAsset), or return nullptr if asset does not exist (NullAsset, default)
            template <typename F, typename ReturnedAsset = NullAsset>
            std::shared_ptr<F> GetAsset() const
            {
                io::VirtualTransportSystem& vts = BLobLoader<T>::mVTS;
                if (auto asset = GetAsset())
                {
                    std::shared_ptr<F> forcedAsset = std::make_shared<F>(asset->mTag, asset->mBuffer, vts);
                    return forcedAsset;
                }

                if constexpr (std::is_same<ReturnedAsset, NullAsset>::value)
                {
                    return nullptr;
                }
                else
                {
                    return std::make_shared<F>(io::Tag{}, yaget::io::Buffer{}, vts);
                }
            }

            template <typename F>
            F GetAsset(std::function<F(typename BLobLoader<T>::AssetPtr baseAsset)> converter) const
            {
                auto asset = GetAsset();
                return converter(asset);
            }

        };


        ////-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
        ////! Provides two notifications, one when section is fully loaded, call functor.
        ////! Second will block in ctor until section is fully loaded.
        //class SectionNotifier
        //{
        //public:
        //    using Sections = io::VirtualTransportSystem::Sections;
        //    using DoneCallback = std::function<void()>;
        //    using AssetCallback = io::VirtualTransportSystem::BlobAssetCallback;

        //    //! Monitors that all assets under section/blobName are loaded and calls doneCallback
        //    SectionNotifier(io::VirtualTransportSystem& vts, const Sections& sections, DoneCallback doneCallback, AssetCallback assetCallback)
        //        : mVts(vts)
        //        , mCountFiles(mVts.GetNumTags(sections))
        //        , mSections(sections)
        //        , mDoneCallback(doneCallback)
        //        , mAssetCallback(assetCallback)
        //    {
        //        if (mCountFiles)
        //        {
        //            mVts.RequestBlob(sections, [this](auto&&... params) { onBlobLoaded(params...); }, nullptr);
        //        }
        //        else if (mDoneCallback)
        //        {
        //            mDoneCallback();
        //        }
        //    }

        //    //! Blocks execution until all assets are loaded under section/filter
        //    SectionNotifier(io::VirtualTransportSystem& vts, const Sections& sections) : SectionNotifier(vts, sections, nullptr, nullptr)
        //    {
        //        platform::Sleep([this]() { return mCountFiles != 0; });
        //    }

        //private:
        //    void onBlobLoaded(std::shared_ptr<io::Asset> asset)
        //    {
        //        if (mAssetCallback && mCountFiles)
        //        {
        //            mAssetCallback(asset);
        //        }

        //        mCountFiles--;
        //        if (mCountFiles == 0 && mDoneCallback)
        //        {
        //            mDoneCallback();
        //        }
        //    }

        //    io::VirtualTransportSystem& mVts;
        //    std::atomic_size_t mCountFiles;
        //    Sections mSections;
        //    DoneCallback mDoneCallback;
        //    AssetCallback mAssetCallback;
        //};

        //-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
        // inline implementation of VirtualTransportSystem
        template<typename A>
        size_t VirtualTransportSystem::RequestBlob(const Section& section, std::function<void(std::shared_ptr<A>)> blobAssetCallback, std::atomic_size_t* tagsCounter)
        {
            return RequestBlob<A>(Sections{ section }, blobAssetCallback, tagsCounter);
        }

        template<typename A>
        size_t VirtualTransportSystem::RequestBlob(const Sections& sections, std::function<void(std::shared_ptr<A>)> blobAssetCallback, std::atomic_size_t* tagsCounter)
        {
            // wrap typed callback into VTS expected one, and then cast and call user typed callback
            auto callback = [blobAssetCallback](std::shared_ptr<io::Asset> asset)
            {
                std::shared_ptr<A> castAsset = io::asset_cast<A>(asset);
                blobAssetCallback(castAsset);
            };

            return RequestBlob(sections, callback, tagsCounter);
        }

        template<typename A>
        size_t VirtualTransportSystem::RequestBlob(const io::Tag& tag, std::function<void(std::shared_ptr<A>)> blobAssetCallback, std::atomic_size_t* tagsCounter)
        {
            return RequestBlob<A>(std::vector<io::Tag>{ tag }, blobAssetCallback, tagsCounter);
        }

        template<typename A>
        size_t VirtualTransportSystem::RequestBlob(const std::vector<io::Tag>& tags, std::function<void(std::shared_ptr<A>)> blobAssetCallback, std::atomic_size_t* tagsCounter)
        {
            // wrap typed callback into VTS expected one, and then cast and call user typed callback
            auto callback = [blobAssetCallback](std::shared_ptr<io::Asset> asset)
            {
                std::shared_ptr<A> castAsset = io::asset_cast<A>(asset);
                blobAssetCallback(castAsset);
            };

            return RequestBlob(tags, callback, tagsCounter);
        }

        //--------------------------------------------------------------------------------------------------
        std::string NormalizePath(const std::string& filePath);

    } // namespace io

    namespace conv
    {
        //-------------------------------------------------------------------------------------------------------------------------------
        template <>
        struct Convertor<io::VirtualTransportSystem::Section>
        {
            static std::string ToString(const io::VirtualTransportSystem::Section& value)
            {
                if (std::string result = value.ToString(); !result.empty())
                {
                    return result;
                }
                else
                {
                    return "NULL";
                }
            }
        };
    }

} // namespace yaget
