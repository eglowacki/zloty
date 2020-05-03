///////////////////////////////////////////////////////////////////////
// AssetLoadSystem.h
//
//  Copyright 11/19/2006 Edgar Glowacki.
//
//  Maintained by: Edgar
//
//  NOTES:
//      This provides singleton for requesting assets
//
//
//  #include "File/AssetLoadSystem.h"
//
//////////////////////////////////////////////////////////////////////
//! \file

#ifndef FILE_ASSET_LOAD_SYSTEM_H
#define FILE_ASSET_LOAD_SYSTEM_H
#pragma once


#include "Hash/Hash.h"
#include "VirtualFileFactory.h"
#include "Exception/Exception.h"
#include "safe_bool.h"
#include <map>
#include <set>
#include <boost/signals.hpp>
#include <boost/tuple/tuple.hpp>
#include <boost/tuple/tuple_comparison.hpp>


namespace eg
{
    class IAsset;
    class Message;
    class Dispatcher;
    class VirtualFileSystem;
    class IdGameCache;

    /*!
    This is used with JobQueue
    where <0> is asset name
    and <1> is asset type
    */
    typedef boost::tuple<std::string, guid_t> AssetData_t;

    /*!
    Asset loading system implemented as a Singleton pattern (using loki)
    This will return cached asset, or create one if it's not cached yet.
    \code
    boost::shared_ptr<MyAsset> my_asset = als.GetAsset<MyAsset>("nymphes.tga");
    bool result = als.SaveAsset<MyAsset>("nymphes.tga");
    \endcode
    */
    class AssetLoadSystem
    {
        // no copy semantics
        AssetLoadSystem(const AssetLoadSystem&);
        AssetLoadSystem& operator=(const AssetLoadSystem&);

    public:
        //! All dealings are done trough shared pointer to asset
        typedef boost::shared_ptr<IAsset> IAsset_t;

        /*!
        Callback to create new asset
        \param const std::string& name of an asset to create
        \return new instance of an asset. Return NULL if did not create new asset.
        */
        typedef boost::function<IAsset *(const std::string& /*name*/, uint64_t /*asssetId*/)> fNewAsset;

        /*!
        This is called when there is new data ready for this asset
        \param IAsset_t instance of asset getting this stream
        \param istream_t input (read only) stream with data
        \param streamName name of this istream_t, since some asset work on wild card
                          specification, and there could be several stream.
        */
        typedef boost::function<void (IAsset_t /*asset*/, VirtualFileFactory::istream_t /*stream*/, const std::string& /*name*/)> fLoadDataAsset;
        typedef boost::function<void (IAsset_t /*asset*/, VirtualFileFactory::ostream_t /*stream*/, const std::string& /*name*/)> fSaveDataAsset;
        typedef boost::function<std::vector<std::string> (IAsset_t /*asset*/, const VirtualFileSystem& /*vfs*/)> fQueryDataAsset;

        AssetLoadSystem(VirtualFileSystem& vfs, IdGameCache& idCache, Dispatcher& dsp);
        virtual ~AssetLoadSystem();

        //! Used as param in GetAsset
        enum Load { eLoadAsync, eLoadBlocking };
        /*!
        Return asset of specific type and load data from persistent storage if one exists
        */
        template <typename T>
        boost::shared_ptr<T> GetAsset(const std::string& name, Load loadType = eLoadAsync);
        template <typename T>
        boost::shared_ptr<T> GetAsset(uint64_t aId, Load loadType = eLoadAsync);

        template <typename T>
        void ReloadAsset(const std::string& name, Load loadType = eLoadAsync);
        template <typename T>
        void ReloadAsset(uint64_t aId, Load loadType = eLoadAsync);

        /*!
        This will return list of all asset names fitting filter
        \param filter wild card filter to get assets
        \return vector of strings which are asset names. This will not have any path only asset names
        */
        //std::vector<std::string> GetAssetList(const std::string& filter) const;

        //! Save asset to persistent storage
        template <typename T>
        bool SaveAsset(const std::string& name);

        //! Save asset to persistent storage
        template <typename T>
        bool SaveAsset(uint64_t aId);

        /*!
        Register specific factory for creation of assets
        \param type asset type to create
        \param newAsset callback for creation of new asset
        \param loadDataAsset callback for loading data
        \param saveDataAsset callback for saving data
        */
        void RegisterFactory(guid_t type, fNewAsset newAsset, fLoadDataAsset loadDataAsset, fSaveDataAsset saveDataAsset, fQueryDataAsset queryDataAsset);
        void UnregisterFactory(guid_t type);

        //! Return readable asset name
        std::string GetAssetName(const IAsset *pAsset) const;

        //! Return readable asset name for this unique hash
        std::string GetAssetName(uint64_t sourceId) const;

        //! simple accessor for underlying file system
        VirtualFileSystem& vfs() {return mVirtualFileSystem;}

    private:
        void onAssetStreamChanged(const std::string& name, VirtualFileSystem&);
        boost::signals::connection mVFSConnection;

        //! Generic get asset method
        IAsset_t getAsset(const std::string& name, guid_t type);

        void reloadAsset(const std::string& name, guid_t type);

        //! Generic save asset method
        bool saveAsset(const std::string& name, guid_t type);

        //! Called by pool thread thread when data stream is loaded and ready to be parsed.
        void onDataLoaded(AssetData_t assetData);

        bool isFilterMatch(const std::string& assetName) const;

        //! all loaded assets (cached)
        typedef std::pair<IAsset_t, bool> Resource_t;
        //! assets are keyd on their name and type
        typedef std::pair<std::string, guid_t> AKey_t;
        typedef std::map<AKey_t, Resource_t> AssetList_t;
        AssetList_t mAssetList;

        /*!
        This represents factory which provides 3 callback for creating new assets,
        load and save stream data
        \note There might be more callback added.
        */
        struct Factory
        {
            Factory(fNewAsset newAsset, fLoadDataAsset loadDataAsset, fSaveDataAsset saveDataAsset, fQueryDataAsset queryDataAsset)
            : mNewAsset(newAsset)
            , mLoadDataAsset(loadDataAsset)
            , mSaveDataAsset(saveDataAsset)
            , mQeryDataAsset(queryDataAsset)
            {}

            fNewAsset mNewAsset;
            fLoadDataAsset mLoadDataAsset;
            fSaveDataAsset mSaveDataAsset;
            fQueryDataAsset mQeryDataAsset;
        };

        //! Return factory for given type if there exist one, otherwise NULL
        Factory *FindFactory(guid_t type) const;

        //! All registered factories
        typedef boost::shared_ptr<Factory> Factory_t;
        typedef std::map<guid_t, Factory_t> FactoryList_t;
        FactoryList_t mFactoryList;

        VirtualFileSystem& mVirtualFileSystem;
        IdGameCache& mIdCache;
        Dispatcher& mDispatcher;

        void reloadAsset(const std::string& name);
    };


    //-------------------------------------------------------------------------------------------------
    //-------------------------------------------------------------------------------------------------
    //-------------------------------------------------------------------------------------------------
    template <typename T>
    inline boost::shared_ptr<T> AssetLoadSystem::GetAsset(const std::string& name, Load /*loadType*/)
    {
        IAsset_t asset = getAsset(name, T::kType);
        boost::shared_ptr<T> typedAsset = boost::dynamic_pointer_cast<T>(asset);
        return typedAsset;
    }


    //-------------------------------------------------------------------------------------------------
    template <typename T>
    inline boost::shared_ptr<T> AssetLoadSystem::GetAsset(uint64_t aId, Load loadType)
    {
        return GetAsset<T>(GetAssetName(aId), loadType);
    }

    template <typename T>
    inline void AssetLoadSystem::ReloadAsset(const std::string& name, Load /*loadType*/)
    {
        reloadAsset(name, T::kType);
    }

    template <typename T>
    inline void AssetLoadSystem::ReloadAsset(uint64_t aId, Load loadType)
    {
        ReloadAsset<T>(GetAssetName(aId), loadType);
    }

    //-------------------------------------------------------------------------------------------------
    template <typename T>
    inline bool AssetLoadSystem::SaveAsset(const std::string& name)
    {
        return saveAsset(name, T::kType);
    }

    //-------------------------------------------------------------------------------------------------
    template <typename T>
    inline bool AssetLoadSystem::SaveAsset(uint64_t aId)
    {
        return saveAsset<T>(GetAssetName(aId));
    }


    //! Used to specify what kind of loading specific asset should use
    struct lod_type_bloc {static const int load = static_cast<int>(AssetLoadSystem::eLoadBlocking);};
    struct lod_type_async {static const int load = static_cast<int>(AssetLoadSystem::eLoadAsync);};
    typedef lod_type_bloc lod_type_default;


    //-------------------------------------------------------------------------------------
    //! Helper class which provides handle like behavior and encapsulates
    //! usage of AssetLoadSystem
    template <typename T, typename L = lod_type_default>
    class hAsset : public safe_bool<hAsset<T, L> >
    {
        // no copy semantics
        hAsset(const hAsset&);
        hAsset& operator=(const hAsset&);

    public:
        hAsset(AssetLoadSystem& als)
        : mALS(als)
        {
        }

        hAsset(const std::string& name, AssetLoadSystem& als)
        : mALS(als)
        , mAsset(als.GetAsset<T>(name, static_cast<AssetLoadSystem::Load>(L::load)))
        {
            if (!mAsset)
            {
                log_error << "Did not get asset '" << name << "'.";
                throw ex::resource("Did not get asset");
            }
        }

        void operator=(const std::string& name)
        {
            if (mAsset && mAsset->Name() == name)
            {
                return;
            }

            mAsset = mALS.GetAsset<T>(name, static_cast<AssetLoadSystem::Load>(L::load));
            if (!mAsset && !name.empty())
            {
                log_error << "Did not get asset '" << name << "'.";
                throw ex::resource("Did not get asset");
            }
        }

        void operator*=(const std::string& name)
        {
            if (mAsset && mAsset->Name() == name)
            {
                mALS.ReloadAsset<T>(name, static_cast<AssetLoadSystem::Load>(L::load));
            }
            else
            {
                (*this) = name;
            }
        }

        T *operator->() const
        {
            if (!mAsset)
            {
                log_error << "Asset pointer is NULL.";
                throw ex::resource("Asset pointer is NULL.");
            }

            return mAsset.get();
        }

        void save() const
        {
            if (!mAsset)
            {
                log_error << "Asset pointer is NULL.";
                throw ex::resource("Asset pointer is NULL.");
            }
            mALS.SaveAsset<T>(mAsset->Name());
        }

        const std::string& name() const
        {
            if (mAsset)
            {
                return mAsset->Name();
            }

            log_error << "Asset pointer is NULL.";
            static std::string empty("{NULL}");
            return empty;
        }

        //! no throw of getting raw pointer
        const T *get() const {return mAsset.get();}

    //protected:
        bool boolean_test() const
        {
            return mAsset.get() != 0;
        }

    private:
        AssetLoadSystem& mALS;
        boost::shared_ptr<T> mAsset;
    };


} // namespace eg


#define tr_asset "asset"

#endif // FILE_ASSET_LOAD_SYSTEM_H

