#include "File/AssetLoadSystem.h"
#include "Asset/IAsset.h"
#include "Hash/Hash.h"
#include "IdGameCache.h"
#include "Message/Dispatcher.h"
#include "File/VirtualFileSystem.h"
#include "MessageInterface.h"
#include "StringHelper.h"
#include "Exception/Exception.h"
#include <boost/bind.hpp>
#include <boost/foreach.hpp>
#include <boost/function.hpp>
#include <boost/filesystem.hpp>


namespace bfs = boost::filesystem;

namespace
{
} // namespace

namespace eg {


AssetLoadSystem::AssetLoadSystem(VirtualFileSystem& vfs, IdGameCache& idCache, Dispatcher& dsp)
: mVirtualFileSystem(vfs)
, mIdCache(idCache)
, mDispatcher(dsp)
{
    mVFSConnection = mVirtualFileSystem.StreamChangedEvent.connect(boost::bind(&AssetLoadSystem::onAssetStreamChanged, this, _1, _2));

    log_trace(tr_asset) << "AssetLoadSystem object created.";
}


AssetLoadSystem::~AssetLoadSystem()
{
    mVFSConnection.disconnect();
    if (!mAssetList.empty())
    {
        log_error << "AssetLoadSystem object still has " << mAssetList.size() << " assets left on destruction.";
        for (AssetList_t::const_iterator it = mAssetList.begin(); it != mAssetList.end(); ++it)
        {
            log_trace(tr_asset) << "  Name '" << (*it).first.first << "', Type '" << logs::hex<guid_t>((*it).first.second) << "'.";
        }
    }
    log_trace(tr_asset) << "AssetLoadSystem object deleted.";
}



/*
std::vector<std::string> AssetLoadSystem::GetAssetList(const std::string& filter) const
{
    std::vector<std::string> result = mVirtualFileSystem.GetFileList(filter);
    return result;
}
*/


void AssetLoadSystem::reloadAsset(const std::string& name)
{
    BOOST_FOREACH(const AssetList_t::value_type& asset, mAssetList)
    {
        std::string assetFqn(asset.second.first->Fqn());
        if (assetFqn.empty())
        {
            continue;
        }
        std::string fileFqn(name);
        assetFqn = NormalizePath(assetFqn, false, false);
        assetFqn = "*" + assetFqn;
        fileFqn = NormalizePath(fileFqn, false, false);

        if (WildCompareI(assetFqn, fileFqn))
        {
            // get asset name and it's factory type
            AssetData_t assetData(asset.second.first->Name(), asset.first.second);
            onDataLoaded(assetData);
            break;
        }
    }
}

void AssetLoadSystem::onAssetStreamChanged(const std::string& name, VirtualFileSystem&)
{
    next_tick dataChanged(mDispatcher, boost::bind(&AssetLoadSystem::reloadAsset, this, name));
}


AssetLoadSystem::IAsset_t AssetLoadSystem::getAsset(const std::string& name, guid_t type)
{
    // \todo let's validate name hee first...

    Factory *pFactory = 0;
    AssetLoadSystem::IAsset_t asset;

    // find asset in the list if it exist
    AssetList_t::const_iterator it = mAssetList.find(AKey_t(name, type));
    if (it == mAssetList.end())
    {
        pFactory = FindFactory(type);
        if (!pFactory)
        {
            log_error << "Did not find factory for '" << type << "' type requested by '" << name << "' asset.";
            return IAsset_t();
        }
    }
    else
    {
        asset = (*it).second.first;
        if (!asset)
        {
            log_error << "There is no instance of '" << name << "' asset, but has entry in the collection.";
        }
    }

    // there is no asset created yet
    if (!asset && pFactory)
    {
        uint64_t asssetId = idspace::get_burnable(mIdCache);
        if (IAsset *pAsset = pFactory->mNewAsset(name, asssetId))
        {
            asset.reset(pAsset);
            mAssetList.insert(std::make_pair(AKey_t(name, type), std::make_pair(asset,  false)));

            // trigger loading of data, but only if this is the first time we created this asset
            AssetData_t assetData(name, type);
            onDataLoaded(assetData);
        }
    }

    log_trace(tr_asset) << "Asset '" << name << "' of type: " << logs::hex<guid_t>(type) << " requested.";
    return asset;
}


void  AssetLoadSystem::reloadAsset(const std::string& name, guid_t type)
{
    AssetList_t::const_iterator it = mAssetList.find(AKey_t(name, type));
    if (it != mAssetList.end())
    {
        AssetData_t assetData(name, type);
        onDataLoaded(assetData);
    }
}


void AssetLoadSystem::onDataLoaded(AssetData_t assetData)
{
    IAsset_t asset;
    Factory *pFactory = 0;
    AssetList_t::iterator ita = mAssetList.find(AKey_t(assetData.get<0>(), assetData.get<1>()));
    if (ita != mAssetList.end())
    {
        asset = (*ita).second.first;
        pFactory = FindFactory(asset->Type());
        if (!pFactory)
        {
            log_error << "Could not find '" << asset->Type() << "' Factory for '" << asset->Name() << "' asset.";
            return;
        }
    }
    else
    {
        // this can happened if asset was deleted, after load request was issued
        // but before this method got called. In this case we do nothing and return immediately.
        log_warning << "Asset '" << assetData.get<0>() << "' was removed while waiting on Data Load.";
        return;
    }

    if (pFactory && asset)
    {
        try
        {
            std::vector<std::string> assetFiles = pFactory->mQeryDataAsset(asset, mVirtualFileSystem);
            for (std::vector<std::string>::const_iterator it = assetFiles.begin(); it != assetFiles.end(); ++it)
            {
                if (VirtualFileFactory::istream_t dataStream = mVirtualFileSystem.GetFileStream(*it))
                {
                    pFactory->mLoadDataAsset(asset, dataStream, *it);
                }
            }

            // and since there were more then one file, we need to send blank one to notify
            // that we are finished with loading
            pFactory->mLoadDataAsset(asset, VirtualFileFactory::istream_t(), asset->Name());

            log_trace(tr_asset) << "Asset '" << asset->Name() << "' of type: " << logs::hex<guid_t>(asset->Type()) << " loaded.";
        }
        catch (const ex::serialize& e)
        {
            log_error << "Could not load asset '" << asset->Name() << "' of type: " << logs::hex<guid_t>(asset->Type()) << ". Exception: " << e.what();
        }
    }
}


bool AssetLoadSystem::isFilterMatch(const std::string& /*assetName*/) const
{
    return true;
}


bool AssetLoadSystem::saveAsset(const std::string& name, guid_t type)
{
    IAsset_t asset ;
    Factory *pFactory = 0;
    AssetList_t::const_iterator it = mAssetList.find(AKey_t(name, type));

    if (it != mAssetList.end())
    {
        asset = (*it).second.first;
        pFactory = FindFactory(type);
        if (!pFactory)
        {
            log_error << "Could not find '" << type << "' Factory for '" << name << "' asset.";
            return false;
        }
    }

    if (pFactory && asset)
    {
        try
        {
            std::vector<std::string> assetFiles = pFactory->mQeryDataAsset(asset, mVirtualFileSystem);
            for (std::vector<std::string>::const_iterator it = assetFiles.begin(); it != assetFiles.end(); ++it)
            {
                if (VirtualFileFactory::ostream_t dataStream = mVirtualFileSystem.AttachFileStream(*it))
                {
                    pFactory->mSaveDataAsset(asset, dataStream, *it);
                }
            }

            // and since there were more then one file, we need to send blank one to notify
            // that we are finished with loading
            pFactory->mSaveDataAsset(asset, VirtualFileFactory::ostream_t(), asset->Name());
            return true;
        }
        catch (const ex::serialize& e)
        {
            log_error << "Could not save asset '" << name << "' of type: " << logs::hex<guid_t>(type) << ". Exception: " << e.what();
        }
    }
    else
    {
        log_warning << "Saving '" << name << "' asset failed, does not have a record.";
    }

    return false;
}


void AssetLoadSystem::RegisterFactory(guid_t type, fNewAsset newAsset, fLoadDataAsset loadDataAsset, fSaveDataAsset saveDataAsset, fQueryDataAsset queryDataAsset)
{
    if (mFactoryList.find(type) != mFactoryList.end())
    {
        log_error << "Factory '" << type << "' already exists, new one will be used.";
    }

    log_trace(tr_asset) << "Factory of type " << logs::hex<guid_t>(type) << " registered.";
    mFactoryList.insert(std::make_pair(type, new Factory(newAsset, loadDataAsset, saveDataAsset, queryDataAsset)));
}


void AssetLoadSystem::UnregisterFactory(guid_t type)
{
    log_trace(tr_asset) << "Factory of type " << logs::hex<guid_t>(type) << " un-registered.";

    for (AssetList_t::const_iterator it = mAssetList.begin(); it != mAssetList.end();)
    {
        if ((*it).first.second == type)
        {
            mAssetList.erase(it++);
        }
        else
        {
            ++it;
        }
    }

    mFactoryList.erase(type);
}


AssetLoadSystem::Factory *AssetLoadSystem::FindFactory(guid_t type) const
{
    FactoryList_t::const_iterator it = mFactoryList.find(type);
    if (it != mFactoryList.end())
    {
        return (*it).second.get();
    }

    return 0;
}


std::string AssetLoadSystem::GetAssetName(const IAsset *pAsset) const
{
    if (pAsset)
    {
        return GetAssetName(pAsset->GetId());
    }

    return std::string();
}


std::string AssetLoadSystem::GetAssetName(uint64_t sourceId) const
{
    for (AssetList_t::const_iterator it = mAssetList.begin(); it != mAssetList.end(); ++it)
    {
        uint64_t aId = (*it).second.first.get()->GetId();
        if (aId == sourceId)
        {
            return (*it).first.first;
        }
    }

    return std::string();
}



} // namespace eg

