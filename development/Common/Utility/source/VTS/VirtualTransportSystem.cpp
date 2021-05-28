#include "VTS/VirtualTransportSystem.h"
#include "App/AppUtilities.h"
#include "Logger/YLog.h"
#include "App/FileUtilities.h"
#include "Exception/Exception.h"
#include "Metrics/Concurrency.h"
#include "Streams/Buffers.h"
#include "Metrics/Concurrency.h"

#include <filesystem>
#include <utility>
namespace fs = std::filesystem;

#define YAGET_VTS_VERSION 10

#include "VirtualTransportSystemCollector.inl"


yaget::io::VirtualTransportSystem::VirtualTransportSystem(dev::Configuration::Init::VTSConfigList configList, VirtualTransportSystem::DoneCallback doneCallback, const AssetResolvers& assetResolvers, const std::string& fileName, RuntimeMode reset)
    : mRuntimeMode(RuntimeMode::Optimum)
    , mDoneCallback(std::move(doneCallback))
    , mRequestPool("vts.Request", dev::CurrentConfiguration().mDebug.mThreads.VTS)
    , mAssetResolvers(assetResolvers)
    , mDatabase(ResolveDatabaseName(fileName, reset == RuntimeMode::Reset), vtsSchema, YAGET_VTS_VERSION)
    , mSectionEntriesCollector(std::make_shared<SectionEntriesCollector>(configList, mDatabase, [this]() { onEntriesCollected(); }))
    , mBlobLoader(false, [this](auto&&... params) { onErrorBlobLoader(params...); })
{
}


yaget::io::VirtualTransportSystem::VirtualTransportSystem(RuntimeMode runtimeMode, const std::string& fileName)
    : mRuntimeMode(runtimeMode)
    , mRequestPool("vts.Request", 1)
    , mDatabase(ResolveDatabaseName(fileName, false), vtsSchema, YAGET_VTS_VERSION)
    , mBlobLoader(false, [this](auto&&... params) { onErrorBlobLoader(params...); })
{
}


yaget::io::VirtualTransportSystem::~VirtualTransportSystem()
{
    const double MaxTimeToWait = 5.0;

    double startDestroyTime = platform::GetRealTime();
    platform::Sleep([this, startDestroyTime, MaxTimeToWait]()
    { 
        std::size_t counter = mBlobLoader.CurrentCounter();

        double nowTime = platform::GetRealTime();
        if (nowTime - startDestroyTime > MaxTimeToWait)
        {
            YAGET_ASSERT(false, "Waiting for mBlobLoader to finish on files: '%d' from VTS dtor.", counter);
            return false;
        }

        return counter != 0;
    });

    if (mRuntimeMode == RuntimeMode::Optimum)
    {
        using DirtyRow = std::tuple<Guid /*guid*/, std::string /*VTS*/, std::string /*Section*/>;
        std::vector<DirtyRow> dirtyBlobs = mDatabase.DB().GetRowsTuple<DirtyRow>("SELECT Tags.Guid, Tags.VTS, Tags.Section FROM Tags INNER JOIN DirtyTags ON Tags.Guid=DirtyTags.Guid;");

        for (const auto& it : dirtyBlobs)
        {
            io::Tag tag;
            tag.mGuid = std::get<0>(it);
            auto asset = FindAsset(tag);
            YAGET_ASSERT(asset, "Did not find asset: '%s' in collection while trying to save dirty blob.", std::get<1>(it).c_str());

            std::string fileName = util::ExpendEnv(std::get<1>(it), nullptr);
            if (!fs::path(fileName).has_extension())
            {
                std::string command = fmt::format("SELECT Filters FROM 'Sections' WHERE Name = '{}';", std::get<2>(it));
                auto filters = GetCell<Strings>(mDatabase.DB(), command);
                if (filters.size() == 1)
                {
                    auto extension = fs::path(*filters.begin()).extension().generic_string();
                    fileName = fs::path(fileName).replace_extension(extension).generic_string();
                }
            }

            bool result = mBlobLoader.Save(asset->mBuffer, fileName);
            YAGET_ASSERT(result, "Did not write out file: '%s'.", fileName.c_str());
        }

        bool result = mDatabase.DB().ExecuteStatement("DELETE FROM 'DirtyTags';", nullptr);
        YAGET_ASSERT(result, "Did not delete 'DirtyTags' table.");
    }

    mDatabase.DB().Log("SESSION_END", "VTS Ended");
}


void yaget::io::VirtualTransportSystem::onEntriesCollected()
{
    mSectionEntriesCollector = nullptr;
    mDoneCallback();
}


void yaget::io::VirtualTransportSystem::onErrorBlobLoader(const std::string& filePathName, const std::string& errorMessage)
{
    YLOG_WARNING("VTS", "Blob '%s' failed to load. %s.", filePathName.c_str(), errorMessage.c_str());
};


yaget::io::VirtualTransportSystem::AssetResolver yaget::io::VirtualTransportSystem::FindAssetConverter(const std::string& converterType) const
{
    if (auto it = mAssetResolvers.find(converterType); it!= std::end(mAssetResolvers))
    {
        return it->second;
    }

    return {};
}

void yaget::io::VirtualTransportSystem::onBlobLoaded(const io::Buffer& dataBuffer, const io::Tag& requestedTag, BlobAssetCallback assetLoaded, std::atomic_size_t* tagsCounter)
{
    metrics::Channel span(fmt::format("BlobLoaded {}", requestedTag.mVTSName).c_str(), YAGET_METRICS_CHANNEL_FILE_LINE);

    TagCounterKeeper tagCounterKeeper(tagsCounter, requestedTag);

    try
    {
        std::shared_ptr<Asset> asset = FindAsset(requestedTag);
        if (!asset)
        {
            // incoming data blob, find converter callback for it and execute
            const std::string command = fmt::format("SELECT Sections.Converters FROM Sections INNER JOIN Tags ON Tags.Guid = '{}' AND Sections.Name = Tags.Section;", requestedTag.mGuid.str());
            std::string converterType;

            if (DatabaseHandle dHandle = LockDatabaseAccess())
            {
                converterType = GetCell<std::string>(dHandle->DB(), command);
            }

            auto converter = FindAssetConverter(converterType);
            YAGET_ASSERT(converter, "Asset Resolvers section '%s' does not have entry for: '%s'. Requested tag: '%s', Expended: '%s'.",
                requestedTag.mSectionName.c_str(),
                converterType.c_str(),
                requestedTag.mVTSName.c_str(),
                requestedTag.ResolveVTS().c_str());

            if (auto newAsset = converter(dataBuffer, requestedTag, *this))
            {
                std::unique_lock<std::mutex> locker(mMutexAssets);
                asset = FindAssetNonMT(requestedTag);
                if (!asset)
                {
                    asset = AddAssetNonMT(newAsset);
                }
            }
        }

        if (asset)
        {
            assetLoaded(asset);
        }
    }
    catch (const yaget::ex::standard& e)
    {
        const std::string message = fmt::format("Blob '{}' did not get converted. '{}'.", requestedTag.mVTSName.c_str(), e.what());
        YLOG_ERROR("VTS", message.c_str());
    }
}


//--------------------------------------------------------------------------------------------------
std::shared_ptr<yaget::io::Asset> yaget::io::VirtualTransportSystem::AddAsset(const std::shared_ptr<yaget::io::Asset>& asset)
{
    // due to mt nature of vts, the query for asset returning null and then later creating that new asset
    // can be interrupted in the middle and insert that asset, making "first" creation duplicate
    std::unique_lock<std::mutex> locker(mMutexAssets);
    return AddAssetNonMT(asset);
}


//--------------------------------------------------------------------------------------------------
std::shared_ptr<yaget::io::Asset> yaget::io::VirtualTransportSystem::AddAssetNonMT(const std::shared_ptr<yaget::io::Asset>& asset)
{
    auto result = mAssets.insert(std::make_pair(asset->mTag, asset));
    YAGET_ASSERT(result.second, "Asset: '%s' already exists in collection.", asset->mTag.mVTSName.c_str());

    return result.first->second;
}


//--------------------------------------------------------------------------------------------------
void yaget::io::VirtualTransportSystem::ClearAssets(const io::Tags& tags)
{
    std::unique_lock<std::mutex> locker(mMutexAssets);
    for (const auto& tag : tags)
    {
        mAssets.erase(tag);
        mOverrideAssets.erase(tag);
    }
}


//--------------------------------------------------------------------------------------------------
void yaget::io::VirtualTransportSystem::RemoveAsset(const io::Tag& tag)
{
    ClearAssets({ tag });
}


//--------------------------------------------------------------------------------------------------
std::shared_ptr<yaget::io::Asset> yaget::io::VirtualTransportSystem::FindAsset(const io::Tag& tag) const
{
    std::unique_lock<std::mutex> locker(mMutexAssets);
    return FindAssetNonMT(tag);
}


//--------------------------------------------------------------------------------------------------
std::shared_ptr<yaget::io::Asset> yaget::io::VirtualTransportSystem::FindAssetNonMT(const io::Tag& tag) const
{
    auto it = mOverrideAssets.find(tag);
    if (it != mOverrideAssets.end())
    {
        return it->second;
    }

    it = mAssets.find(tag);
    if (it != mAssets.end())
    {
        return it->second;
    }

    return std::shared_ptr<io::Asset>();
}


//--------------------------------------------------------------------------------------------------
void yaget::io::VirtualTransportSystem::AddOverride(const std::shared_ptr<yaget::io::Asset>& asset)
{
    std::unique_lock<std::mutex> locker(mMutexAssets);

    YAGET_ASSERT(FindAssetNonMT(asset->mTag), "Trying to add override asset: '%s' that does not exist.", asset->mTag.mVTSName.c_str());

    auto result = mOverrideAssets.insert(std::make_pair(asset->mTag, asset));
    YAGET_ASSERT(result.second, "Asset: '%s' already exists in cashed collection.", asset->mTag.mVTSName.c_str());
}


//--------------------------------------------------------------------------------------------------
bool yaget::io::VirtualTransportSystem::AttachTransientBlob(const std::vector<std::shared_ptr<io::Asset>>& assets)
{
    if (DatabaseHandle databaseHandle = LockDatabaseAccess())
    {
        yaget::db::Transaction transaction(databaseHandle->DB());
        if (!AttachTransientBlobNonMT(assets, transaction))
        {
            return false;
        }
    }

    for (const auto& asset : assets)
    {
        (void)AddAsset(asset);
    }

    return true;
}

bool yaget::io::VirtualTransportSystem::AttachTransientBlobNonMT(const std::vector<std::shared_ptr<io::Asset>>& assets, yaget::db::Transaction& transaction)
{
    using TagRecordTuple = std::tuple<Guid /*Guid*/, std::string /*Name*/, std::string /*VTS*/, std::string /*Section*/>;

    for (const auto& it : assets)
    {
        const io::Tag& tag = it->mTag;
        TagRecordTuple tagRecord(tag.mGuid, tag.mName, tag.mVTSName, tag.mSectionName);

        if (!transaction.DB().ExecuteStatementTuple("TagInsert", "Tags", tagRecord, { "Guid", "Name", "VTS", "Section" }, SQLite::Behaviour::Insert))
        {
            transaction.Rollback();

            std::string message = fmt::format("Attaching blob '{}' to Section: '{}' as VTS: '{}' failed. {}.", tag.mName, tag.mSectionName, tag.mVTSName, ParseErrors(transaction.DB()));
            YLOG_ERROR("VTS", message.c_str());

            return false;
        }
    }

    return true;
}


bool yaget::io::VirtualTransportSystem::UpdateAssetData(const std::shared_ptr<io::Asset>& asset, VirtualTransportSystem::Request request)
{
    if (DatabaseHandle databaseHandle = LockDatabaseAccess())
    {
        SQLite& database = databaseHandle->DB();
        const auto& tag = asset->mTag;

        // verify that we can actually write to this section, Section will be mark ReadOnly = 1
        std::string command = fmt::format("SELECT ReadOnly FROM 'Sections' WHERE Name = '{}';", tag.mSectionName);
        if (GetCell(database, command, true))
        {
            std::string message = fmt::format("Can not update/save asset '{}' data due to section '{}' being marked ReadOnly.", tag.mName, tag.mSectionName);
            YLOG_WARNING("VTS", message.c_str());
            return false;
        }

        const std::string dirtyCommand = fmt::format("INSERT INTO 'DirtyTags' VALUES('{}');", tag.mGuid.str());
        yaget::db::Transaction transaction(database);
        using TagRecordTuple = std::tuple<Guid /*Guid*/, std::string /*Name*/, std::string /*VTS*/, std::string /*Section*/>;

        // lock asset list for any changes
        std::unique_lock<std::mutex> locker(mMutexAssets);
        auto assetData = FindAssetNonMT(tag);
        if (!assetData && request == Request::Add)
        {
            TagRecordTuple tagRecord(tag.mGuid, tag.mName, tag.mVTSName, tag.mSectionName);
            if (!database.ExecuteStatementTuple("TagInsert", "Tags", tagRecord, { "Guid", "Name", "VTS", "Section" }, SQLite::Behaviour::Insert) ||
                !database.ExecuteStatement(dirtyCommand, nullptr))
            {
                transaction.Rollback();

                std::string message = fmt::format("UpdateAssetData for blob '{}' failed. {}.", tag.mName, ParseErrors(database));
                YLOG_WARNING("VTS", message.c_str());

                return false;
            }

            AddAssetNonMT(asset);
            return true;
        }
        else if (!assetData)
        {
            std::string message = fmt::format("Can not update blob '{}' due to no entry in DB.", tag.mName);
            YLOG_WARNING("VTS", message.c_str());

            return false;
        }

        if (!database.ExecuteStatement(dirtyCommand, nullptr))
        {
            transaction.Rollback();
            std::string message = fmt::format("UpdateAssetData for blob '{}' failed. {}.", tag.mName, ParseErrors(database));
            YLOG_WARNING("VTS", message.c_str());

            return false;
        }

        assetData->mBuffer = io::CloneBuffer(asset->mBuffer);
        return true;
    }

    return false;
}

//--------------------------------------------------------------------------------------------------
size_t yaget::io::VirtualTransportSystem::GetNumTags(const Sections& sections) const
{
    size_t numTags = 0;

    if (DatabaseHandle dHandle = LockDatabaseAccess())
    {
        for (const auto& section : sections)
        {
            std::string operation = section.FilterMatchCh[static_cast<int>(section.Match)];
            std::string command = fmt::format("SELECT Path FROM Sections WHERE Name = '{}'", section.Name);
            Strings sectionPath = GetCell<Strings>(dHandle->DB(), command);
            if (!sectionPath.empty())
            {
                for (auto it = sectionPath.rbegin(); it != sectionPath.rend(); ++it)
                {
                    std::string vtsName = *it + "/" + section.Filter;
                    command = io::db::TagRecordQuery(Section(operation + section.Name + "@" + vtsName), "COUNT(*)");
                    size_t nextResults = GetCell<size_t>(dHandle->DB(), command);
                    if (section.Match == Section::FilterMatch::Override)
                    {
                        if (nextResults == 1)
                        {
                            return 1;
                        }
                    }
                    else
                    {
                        numTags += nextResults;
                    }
                }
            }
        }
    }

    return numTags;
}


bool yaget::io::VirtualTransportSystem::IsSectionValid(const Sections& sections) const
{
    if (DatabaseHandle dHandle = LockDatabaseAccess())
    {
        for (const auto& section : sections)
        {
            std::string query = fmt::format("SELECT Name FROM Sections WHERE Name = '{}'", section.Name);
            std::string sectionName = GetCell<std::string>(dHandle->DB(), query);
            if (sectionName.empty())
            {
                return false;
            }
        }
    }

    return !sections.empty();
}


std::vector<yaget::io::Tag> yaget::io::VirtualTransportSystem::GetTags(const Sections& sections) const
{
    std::vector<io::Tag> results;
    if (DatabaseHandle dHandle = LockDatabaseAccess())
    {
        for (const auto& section : sections)
        {
            std::string operation = section.FilterMatchCh[static_cast<int>(section.Match)];

            std::string query = fmt::format("SELECT Path FROM Sections WHERE Name = '{}'", section.Name);
            Strings sectionPath = GetCell<Strings>(dHandle->DB(), query);
            if (!sectionPath.empty())
            {
                for (auto it = sectionPath.rbegin(); it != sectionPath.rend(); ++it)
                {
                    using TagRecordTuple = std::tuple<Guid /*Guid*/, std::string /*Name*/, std::string /*VTS*/, std::string /*Section*/>;

                    std::string vtsName = *it + "/" + section.Filter;
                    query = io::db::TagRecordQuery(Section(operation + section.Name + "@" + vtsName));
                    std::vector<io::Tag> nextResults = dHandle->DB().GetRowsTuple<io::Tag, TagRecordTuple>(query, [](const TagRecordTuple& record)
                    {
                        return io::Tag{std::get<1>(record), std::get<0>(record), std::get<2>(record), std::get<3>(record) };
                    });

                    if (section.Match == Section::FilterMatch::Override)
                    {
                        if (nextResults.size() == 1)
                        {
                            return nextResults;
                        }
                    }
                    else
                    {
                        results.insert(results.end(), nextResults.begin(), nextResults.end());
                    }
                }
            }
        }
    }

    return results;
}


yaget::io::Tag yaget::io::VirtualTransportSystem::GenerateTag(const Section& section) const
{
    using TargetSection = std::tuple<std::string /*Name*/, Strings /*Path*/, Strings /*Filters*/, std::string /*Converters*/, bool /*ReadOnly*/>;

    io::Tag newTag;

    if (DatabaseHandle databaseHandle = LockDatabaseAccess())
    {
        SQLite& database = databaseHandle->DB();
        std::string command = fmt::format("SELECT Name, Path, Filters, Converters, ReadOnly FROM Sections WHERE Name = '{}'", section.Name);

        bool result = true;
        TargetSection targetSection = database.GetRowTuple<TargetSection>(command, &result);
        if (!result)
        {
            YLOG_ERROR("VTS", "Did not get Target Section from section: '%s'. %s", section.ToString().c_str(), ParseErrors(database).c_str());
            return {};
        }

        fs::path filePath = *std::get<1>(targetSection).begin() + "/" + section.Filter;

        newTag.mName = filePath.filename().stem().string();
        newTag.mVTSName = filePath.generic_string();
        newTag.mSectionName = section.Name;

        command = fmt::format("SELECT Guid FROM Deleted WHERE VTS = '{}';", newTag.mVTSName);
        Guid recoveredGuid = GetCell<Guid>(database, command);
        if (recoveredGuid.IsValid())
        {
            command = fmt::format("DELETE FROM Deleted WHERE Guid = '{}';", recoveredGuid.str());
            if (!database.ExecuteStatement(command, nullptr))
            {
                YLOG_ERROR("VTS", "Did not deleted: '%s' from Deleted table. %s", newTag.mVTSName.c_str(), ParseErrors(database).c_str());
            }
            else
            {
                newTag.mGuid = recoveredGuid;
            }
        }
    }

    newTag.mGuid = newTag.mGuid.IsValid() ? newTag.mGuid : NewGuid();
    return newTag;
}


yaget::io::Tag yaget::io::VirtualTransportSystem::AssureTag(const Section& section)
{
    auto tag = GetTag(section);
    if (!tag.IsValid())
    {
        tag = GenerateTag(section);
    }

    return tag;
}



size_t yaget::io::VirtualTransportSystem::RequestBlob(const std::vector<io::Tag>& tags, BlobAssetCallback blobAssetCallback, std::atomic_size_t* tagsCounter)
{

    std::vector<yaget::io::Tag> tagRecords = tags;

    if (!tagRecords.empty())
    {
        if (tagsCounter)
        {
            (*tagsCounter) += tagRecords.size();
        }

        std::vector<std::shared_ptr<io::Asset>> loadedAssets;
        Strings fileNames;
        std::vector<io::BlobLoader::Convertor> convertors;

        // create two arrays, one for already loaded assets,
        // and the second for assets that need to be loaded and converted
        for (const auto& tag : tagRecords)
        {
            metrics::MarkStartTimeSpan(tag.Hash(), fmt::format("Loaded: {}", tag.mVTSName).c_str(), YAGET_METRICS_CHANNEL_FILE_LINE);

            if (auto asset = FindAsset(tag))
            {
                // asset already exist, return this
                loadedAssets.push_back(asset);
            }
            else
            {
                auto converter = [this, tag, blobAssetCallback, tagsCounter = tagsCounter](auto&& param) 
                {
                    onBlobLoaded(param, tag, blobAssetCallback, tagsCounter); 
                    metrics::MarkEndTimeSpan(tag.Hash(), YAGET_METRICS_CHANNEL_FILE_LINE);
                };

                fileNames.push_back(util::ExpendEnv(tag.mVTSName, nullptr));
                convertors.push_back(converter);
            }
        }

        // request blob data and asset conversion, and trigger converter callback for each blob
        if (!fileNames.empty())
        {
            mBlobLoader.AddTask(fileNames, convertors);
        }

        // if assets are already loaded, we still trigger callbacks on separate thread to preserve the same way of handling assets.
        // NOTE: One reason, that any consumption of asset is done on a separate threat, without blocking the logic or render threads
        if (!loadedAssets.empty())
        {
            // trigger thread callback
            mRequestPool.AddTask([blobAssetCallback, loadedAssets, this, tagsCounter = tagsCounter]()
            {
                for (const auto& it : loadedAssets)
                {
                    TagCounterKeeper tagCounterKeeper(tagsCounter, it->mTag);

                    try
                    {
                        blobAssetCallback(it);
                    }
                    catch (const yaget::ex::standard& e)
                    {
                        std::string message = fmt::format("Asset '{}' did not get callbacked. '{}'.", it->mTag.mVTSName.c_str(), e.what());
                        YLOG_ERROR("VTS", message.c_str());
                    }

                    metrics::MarkEndTimeSpan(it->mTag.Hash(), YAGET_METRICS_CHANNEL_FILE_LINE);
                }
            });
        }
    }

    return tagRecords.size();
}


std::string yaget::io::VirtualTransportSystem::GetResolverType(const io::Tag& tag) const
{
    std::string resolverType;

    std::string command = fmt::format("SELECT Sections.Converters FROM Sections INNER JOIN Tags on Tags.Guid = '{}' AND Sections.Name == Tags.Section;", tag.mGuid.str());
    if (DatabaseHandle databaseHandle = LockDatabaseAccess())
    {
        SQLite& database = databaseHandle->DB();
        resolverType = GetCell<std::string>(database, command);
    }

    return resolverType;
}


std::string yaget::io::NormalizePath(const std::string& filePath)
{
    fs::path fsPath(filePath);
    std::string result = fsPath.generic_string();

    auto it = std::unique(result.begin(), result.end(), [](const auto& elemOne, const auto& elemTwo)
    {
        return elemOne == '/' && elemTwo == '/';
    });
    result.resize(std::distance(result.begin(), it));

    return result;
}
