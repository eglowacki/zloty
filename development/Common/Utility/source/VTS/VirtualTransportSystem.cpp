#include "VTS/VirtualTransportSystem.h"
#include "App/AppUtilities.h"
#include "Logger/YLog.h"
#include "App/FileUtilities.h"
#include "Exception/Exception.h"
#include "Metrics/Concurrency.h"
#include "Streams/Buffers.h"
#include "Metrics/Concurrency.h"

#include <filesystem>
namespace fs = std::filesystem;

#define YAGET_VTS_VERSION 10


const int kPoolSize = 0;


namespace yaget
{
    namespace conv
    {
        //template<>
        //struct Convertor<yaget::dev::Configuration::Init::VTSConfigList>
        //{
        //    static std::string ToString(const yaget::dev::Configuration::Init::VTSConfigList& value)
        //    {
        //        value;
        //        return "";
        //        //return fmt::format("[r = {:.2f}, g = {:.2f}, b = {:.2f}, a = {:.2f}]", value.x, value.y, value.z, value.w);
        //    }
        //};

        template<>
        struct Convertor<yaget::dev::Configuration::Init::VTS>
        {
            static std::string ToString(const yaget::dev::Configuration::Init::VTS& value)
            {
                return value.Name;
            }
        };
    }
}


//namespace std
//{
//    inline std::string to_string(const yaget::dev::Configuration::Init::VTSConfigList& /*value*/) { return "FOO"; }
//} // namespace std

namespace
{
    const char* DatabasePathName = "$(DatabaseFolder)/$(AppName)/vts.sqlite";

    std::vector<std::string> vtsSchema =
    {
        #include "VTS/VirtualTransportSystemSchema.sqlite"
    };

    //! Assures that all Path in each configList elements point to a valid folder or file. Removes them from result list if not.
    yaget::dev::Configuration::Init::VTSConfigList ValidateSections(const yaget::dev::Configuration::Init::VTSConfigList& configList)
    {
        // we need to prune file path for each section
        yaget::dev::Configuration::Init::VTSConfigList validSections;

        for (const auto& section : configList)
        {
            yaget::Strings verifiedPath;
            std::copy_if(section.Path.begin(), section.Path.end(), std::back_inserter(verifiedPath), [](const std::string& pathName)
            {
                std::string potentialPath = yaget::util::ExpendEnv(pathName, nullptr);
                if (!potentialPath.empty())
                {
                    if (fs::is_directory(potentialPath) || fs::is_regular_file(potentialPath))
                    {
                        return true;
                    }
                    else if (!fs::path(potentialPath).has_extension())
                    {
                        const auto& [result, errorMessage] = yaget::io::file::AssureDirectories(potentialPath + "/");
                        if (result)
                        {
                            return true;
                        }
                    }
                }

                YLOG_ERROR("VTS", "Path Alias: '%s' expended to: '%s' is not valid, skipping.", pathName.c_str(), potentialPath.c_str());
                return false;
            });

            if (!verifiedPath.empty())
            {
                yaget::dev::Configuration::Init::VTS verifiedSection = section;
                std::swap(verifiedSection.Path, verifiedPath);
                validSections.insert(verifiedSection);
            }
        }

        return validSections;
    }



    inline std::string to_string(const yaget::dev::Configuration::Init::VTSConfigList& /*value*/) { return "FOO"; }


    // collect all files from all sections and make db to match. It calls doneCallback after all indexing is done.
    class SectionEntriesCollector
    {
    public:
        using DoneCallback = yaget::io::VirtualTransportSystem::DoneCallback;

        SectionEntriesCollector(yaget::dev::Configuration::Init::VTSConfigList configList, yaget::Database& database, DoneCallback doneCallback)
            : mTimeSpan("VTS Entries Collector", YAGET_METRICS_CHANNEL_FILE_LINE)
            , mDatabase(database)
            , mDoneCallback(doneCallback)
            , mRequestPool("vts.Collector", yaget::dev::CurrentConfiguration().mDebug.mThreads.VTSSections)
        {
            using namespace yaget;
            using VTS = dev::Configuration::Init;
            using SectionRecord = SQLite::Row<std::string /*Name*/, Strings /*Path*/, Strings /*Filters*/, std::string /*Converters*/, bool /*ReadOnly*/, bool /*Recursive*/>;

            //metrics::MarkStartTimeSpan(reinterpret_cast<std::uintptr_t>(this), "Indexing VTS", YAGET_METRICS_CHANNEL_FILE_LINE);
            metrics::Channel channel("Entries Collector", YAGET_METRICS_CHANNEL_FILE_LINE);

            size_t numChanged = 0;
            // now update db to account for deletion, addition and changes
            VTS::VTSConfigList newSection, deletedSections, changedSections;

            { // block rollback in case of any errors
                db::Transaction transaction(mDatabase.DB());

                if (!mDatabase.DB().ExecuteStatement("DELETE FROM 'Logs';", nullptr))
                {
                    transaction.Rollback();
                    std::string message = fmt::format("Did not delete table 'Logs'. DB Error: ", ParseErrors(mDatabase.DB()));
                    YLOG_WARNING("VTS", message.c_str());
                    throw ex::bad_init(message);
                }

                mDatabase.Log("SESSION_START", "VTS Started");

                if (int numDirtyBlobs = GetCell<int>(mDatabase.DB(), "SELECT COUNT(*) FROM 'DirtyTags';"))
                {
                    transaction.Rollback();
                    std::string message = fmt::format("There are '{}' blobs left in DirtyTags table.", numDirtyBlobs);
                    YLOG_WARNING("VTS", message.c_str());
                    throw ex::bad_init(message);
                }

                auto listSize = configList.size();
                auto originalConfigList = configList;
                configList = ValidateSections(configList);
                if (configList.size() != listSize)
                {
                    // looks like some of the entries did not pass validation. We can not proceed with this data,
                    // since it underlines file system and access. User must fix configuration
                    dev::Configuration::Init::VTSConfigList invalidConfigList;
                    std::set_difference(originalConfigList.begin(), originalConfigList.end(), configList.begin(), configList.end(), std::inserter(invalidConfigList, invalidConfigList.begin()));

                    std::string message = fmt::format("VTS Sections '{}' are not valid. Fix VTS section in configuration file.", conv::Combine(invalidConfigList, ", "));
                    YAGET_UTIL_THROW("VTS", message.c_str());
                }

                for (const auto& it : configList)
                {
                    mCounter += it.Path.size();
                }

                std::string command = fmt::format("SELECT Name, Path, Filters, Converters, ReadOnly, Recursive FROM Sections ORDER BY Name;");
                VTS::VTSConfigList sectionRecords = mDatabase.DB().GetRows<VTS::VTS, SectionRecord, VTS::VTSConfigList>(command, [](const SectionRecord& record) { return VTS::VTS{ record.Result, record.Result1, record.Result2, record.Result3, record.Result4, record.Result5 }; });

                std::set_difference(configList.begin(), configList.end(), sectionRecords.begin(), sectionRecords.end(), std::inserter(newSection, newSection.end()));
                std::set_difference(sectionRecords.begin(), sectionRecords.end(), configList.begin(), configList.end(), std::inserter(deletedSections, deletedSections.end()));
                std::set_intersection(configList.begin(), configList.end(), sectionRecords.begin(), sectionRecords.end(), std::inserter(changedSections, changedSections.end()));

                // and proceed with making db sections match current state of the disk
                for (const auto& it : newSection)
                {
                    SectionRecord section(it.Name, it.Path, it.Filters, it.Converters, it.ReadOnly, it.Recursive);
                    if (!mDatabase.DB().ExecuteStatement("SectionInsert", "Sections", section, SQLite::Behaviour::NoTimeStamp, SQLite::Behaviour::Insert))
                    {
                        transaction.Rollback();
                        std::string message = fmt::format("SectionInsert: '{}' for vts failed. {}.", it.Name, ParseErrors(mDatabase.DB()));
                        YAGET_UTIL_THROW("VTS", message.c_str());
                    }
                }

                for (const auto& it : deletedSections)
                {
                    command = fmt::format("DELETE FROM Tags WHERE Section = '{}';", it.Name);
                    if (!mDatabase.DB().ExecuteStatement(command.c_str(), nullptr))
                    {
                        transaction.Rollback();
                        std::string message = fmt::format("Did not delete tags with section: '{}'. {}.", it.Name, ParseErrors(mDatabase.DB()));
                        YAGET_UTIL_THROW("VTS", message.c_str());
                    }

                    command = fmt::format("DELETE FROM Sections WHERE Name = '{}';", it.Name);
                    if (!mDatabase.DB().ExecuteStatement(command.c_str(), nullptr))
                    {
                        transaction.Rollback();
                        std::string message = fmt::format("Did not delete section: '{}'. {}.", it.Name, ParseErrors(mDatabase.DB()));
                        YAGET_UTIL_THROW("VTS", message.c_str());
                    }
                }

                for (const auto& it : changedSections)
                {
                    const VTS::VTS& record = *sectionRecords.find(VTS::VTS{ it.Name });
                    if (record.Path != it.Path || record.Filters != it.Filters || record.Converters != it.Converters || record.ReadOnly != it.ReadOnly)
                    {
                        numChanged++;
                        SectionRecord section(it.Name, it.Path, it.Filters, it.Converters, it.ReadOnly, it.Recursive);
                        if (!mDatabase.DB().ExecuteStatement("SectionInsert", "Sections", section, SQLite::Behaviour::NoTimeStamp, SQLite::Behaviour::Update))
                        {
                            transaction.Rollback();
                            std::string message = fmt::format("SectionInsert: '{}' for vts failed. {}.", it.Name, ParseErrors(mDatabase.DB()));
                            YAGET_UTIL_THROW("VTS", message.c_str());
                        }
                    }
                }
            }

            mDatabase.Log("INFO", fmt::format("VTS Update Sections - New: {}, Deleted: {}, Changed: {}.", newSection.size(), deletedSections.size(), numChanged));

            if (mCounter == 0)
            {
                // there is no files to be processed
                //UpdateDatabase();
                mRequestPool.AddTask([this]()
                {
                    UpdateDatabase();
                });
            }
            else
            {
                // and finally, trigger 'job pool' to process files in each section
                for (const auto& vtsEntry : configList)
                {
                    using namespace std::placeholders;

                    //const dev::Configuration::Init::VTS& vtsEntry = it;

                    // for each list of files in section.Path[n], call this functor
                    auto updateSection = [this](auto&&... params) { UpdateSection(params...); };

                    // trigger request for to call lambda for each vtsEntry, and then check each Path element for validity, and then calling updateSection with files in that Path[n]
                    mRequestPool.AddTask([this, vtsEntry, updateSection]()
                    {
                        for (auto&& p : vtsEntry.Path)
                        {
                            metrics::Channel channel(fmt::format("Indexing Section: {}", vtsEntry.Name).c_str(), YAGET_METRICS_CHANNEL_FILE_LINE);
                            fs::path proposedPath = util::ExpendEnv(p, nullptr);

                            YAGET_ASSERT(fs::is_directory(proposedPath) || fs::is_regular_file(proposedPath), "Proposed path: '%s' expended from '%s' used in Section: '%s' is not a directory or a file.", proposedPath.generic_string().c_str(), p.c_str(), vtsEntry.Name.c_str());

                            Strings newFileSet;
                            {
                                metrics::Channel channel("Processing found files", YAGET_METRICS_CHANNEL_FILE_LINE);

                                size_t envSize = proposedPath.string().size();
                                {
                                    metrics::Channel channel("Getting Files from disk.", YAGET_METRICS_CHANNEL_FILE_LINE);

                                    newFileSet = io::file::GetFileNames(proposedPath.string(), vtsEntry.Recursive, [&vtsEntry](const std::string& fileName)
                                    {
                                        return std::any_of(vtsEntry.Filters.begin(), vtsEntry.Filters.end(), [&fileName](const std::string& filter) {return WildCompare(filter, fileName); });
                                    });
                                }

                                {
                                    metrics::Channel channel("Transforming found file names.", YAGET_METRICS_CHANNEL_FILE_LINE);

                                    std::transform(newFileSet.begin(), newFileSet.end(), newFileSet.begin(), [p, envSize](const std::string& fileName)
                                    {
                                        std::string newPath = p + std::string(fileName.begin() + envSize, fileName.end());
                                        std::transform(newPath.begin(), newPath.end(), newPath.begin(), [](std::string::value_type c) { return c == '\\' ? '/' : c; });
                                        return newPath;
                                    });
                                }
                            }

                            if (false)//newFileSet.size() == 100000)
                            {
                                mCounter += 1;
                                Strings firstHalf(newFileSet.begin(), newFileSet.begin() + 50000);
                                Strings secondHalf(newFileSet.begin() + 50000, newFileSet.end());

                                updateSection(vtsEntry.Name, firstHalf, p);

                                mRequestPool.AddTask([vtsEntry, updateSection, secondHalf, p]()
                                {
                                    updateSection(vtsEntry.Name, secondHalf, p);
                                });
                            }
                            else
                            {
                                updateSection(vtsEntry.Name, newFileSet, p);
                            }
                        }
                    });
                }
            }
        }

        ~SectionEntriesCollector()
        {
            //yaget::metrics::MarkEndTimeSpan(reinterpret_cast<std::uintptr_t>(this), YAGET_METRICS_CHANNEL_FILE_LINE);
        }

    private:
        void UpdateSection(const std::string& sectionName, const yaget::Strings& entryList, const std::string& proposedPath)
        {
            using namespace yaget;

            {
                metrics::Channel channel(fmt::format("Updated Section with {} files.", entryList.size()).c_str(), YAGET_METRICS_CHANNEL_FILE_LINE);

                std::unique_lock<std::mutex> locker(mSectionMutex);
                SectionKey key = std::make_pair(sectionName, proposedPath);
                Strings& section = mSections[key];

                size_t insertPoint = section.size();
                section.resize(insertPoint + entryList.size());
                std::copy(entryList.begin(), entryList.end(), section.begin() + insertPoint);
                std::inplace_merge(section.begin(), section.begin() + insertPoint, section.end());
            }

            size_t counter = --mCounter;
            if (counter == 0)
            {
                // we processed all entries and ready to update db only after all sections are indexed
                UpdateDatabase();
            }
        }

        void UpdateDatabase()
        {
            using namespace yaget;
            using DeletedRecord = SQLite::Row<Guid /*Guid*/, std::string /*Name*/, std::string /*VTS*/, std::string /*Section*/>;

            metrics::Channel channel("UpdateDatabase", YAGET_METRICS_CHANNEL_FILE_LINE);

            size_t numNewTags = 0, numDeletedTags = 0;

            // any errors during updates to tag system, will result in full rollback
            db::Transaction transaction(mDatabase.DB());
            for (const auto& section : mSections)
            {
                const std::string& nameSection = section.first.first;
                const std::string& namePath = section.first.second;
                const Strings& tagFiles = section.second;

                std::string command = io::db::TagRecordQuery(io::VirtualTransportSystem::Section(nameSection + "@" + namePath));
                Strings tagRecords = mDatabase.DB().GetRows<std::string, io::db::TagRecord, Strings>(command, [](const io::db::TagRecord& record) { return record.Result2; });

                // based on what is on disk (tagFiles) and in db (tagRecords), generate deleted files from disk and new files on disk,
                // so we can update db data
                Strings newTags, deletedTags;
                std::set_difference(tagFiles.begin(), tagFiles.end(), tagRecords.begin(), tagRecords.end(), std::inserter(newTags, newTags.end()));
                std::set_difference(tagRecords.begin(), tagRecords.end(), tagFiles.begin(), tagFiles.end(), std::inserter(deletedTags, deletedTags.end()));

                // update db with added and deleted tags to bring up to pair
                // we also want to preserve deleted files to re-use the same guid if that file come back later
                if (!newTags.empty() || !deletedTags.empty())
                {
                    metrics::Channel channel("New and Deleted", YAGET_METRICS_CHANNEL_FILE_LINE);

                    int deletedRowCount = GetCell<int>(mDatabase.DB(), "SELECT COUNT(*) FROM 'Deleted';");
                    const bool checkDeleted = deletedRowCount > 0;
                    std::vector<io::db::TagRecord> deletedBlobs;

                    if (deletedRowCount < 1000)
                    {
                        // load entire table into memory, since it's a small one, otherwise just query db for each one
                        command = fmt::format("SELECT Guid, Name, VTS, Section FROM Deleted;");
                        deletedBlobs = mDatabase.DB().GetRows<io::db::TagRecord>(command);
                    }
                        
                    for (const std::string& vtsName : newTags)
                    {
                        Guid recoveredGuid;
                        if (checkDeleted)
                        {
                            if (deletedBlobs.empty())
                            {
                                command = fmt::format("SELECT Guid FROM Deleted WHERE VTS = '{}';", vtsName);
                                recoveredGuid = GetCell<Guid>(mDatabase.DB(), command);
                            }
                            else
                            {
                                auto it = std::find_if(deletedBlobs.begin(), deletedBlobs.end(), [&vtsName](const auto& param)
                                {
                                    return param.Result2 == vtsName;
                                });
                                if (it != deletedBlobs.end())
                                {
                                    recoveredGuid = (*it).Result3;
                                    deletedBlobs.erase(it);
                                }
                            }
                        }

                        if (recoveredGuid.IsValid())
                        {
                            command = fmt::format("DELETE FROM Deleted WHERE Guid = '{}';", recoveredGuid.str());
                            if (!mDatabase.DB().ExecuteStatement(command.c_str(), nullptr))
                            {
                                transaction.Rollback();
                                std::string message = fmt::format("Did not deleted: '{}' from Deleted table. {}", vtsName, ParseErrors(mDatabase.DB()));
                                YAGET_UTIL_THROW("VTS", message.c_str());
                            }
                        }
                        else
                        {
                            recoveredGuid = NewGuid();
                        }
                        
                        io::db::TagRecord tag(recoveredGuid, fs::path(vtsName).filename().stem().generic_string(), vtsName, nameSection);
                        if (!mDatabase.DB().ExecuteStatement("TagInsert", "Tags", tag, SQLite::Behaviour::NoTimeStamp, SQLite::Behaviour::Insert))
                        {
                            transaction.Rollback();
                            std::string message = fmt::format("TagInsert: '{}' for vts section: {} failed. {}.", vtsName, nameSection, ParseErrors(mDatabase.DB()));
                            YAGET_UTIL_THROW("VTS", message.c_str());
                        }

                        numNewTags++;
                    }

                    for (const auto& it : deletedTags)
                    {
                        command = io::db::TagRecordQuery(io::VirtualTransportSystem::Section(nameSection + "@" + it));
                        io::db::TagRecord existingTag = mDatabase.DB().GetRow<io::db::TagRecord>(command);
                        if (!existingTag.bValid)
                        {
                            transaction.Rollback();
                            std::string message = fmt::format("VTS: '{}' does not exist in Tags table. {}.", it, ParseErrors(mDatabase.DB()));
                            YAGET_UTIL_THROW("VTS", message.c_str());
                        }

                        command = fmt::format("DELETE FROM Tags WHERE Guid = '{}';", existingTag.Result.str());
                        if (!mDatabase.DB().ExecuteStatement(command.c_str(), nullptr))
                        {
                            transaction.Rollback();
                            std::string message = fmt::format("Did not delete tag: '{}' with section: {}. {}.", it, nameSection, ParseErrors(mDatabase.DB()));
                            YAGET_UTIL_THROW("VTS", message.c_str());
                        }

                        if (!mDatabase.DB().ExecuteStatement("DeletedInsert", "Deleted", existingTag, SQLite::Behaviour::NoTimeStamp, SQLite::Behaviour::Update))
                        {
                            transaction.Rollback();
                            std::string message = fmt::format("DeletedInsert: '{}' for vts 'Deleted' failed with section: {}. {}.", it, nameSection, ParseErrors(mDatabase.DB()));
                            YAGET_UTIL_THROW("VTS", message.c_str());
                        }

                        numDeletedTags++;
                    }
                }
            }

            mDatabase.Log("INFO", fmt::format("VTS Update Tags - New: {}, Deleted: {}.", numNewTags, numDeletedTags));

            // fire callback on separate thread from here, since recipient of this message will delete us
            std::thread notifier = std::thread([](DoneCallback doneCallback) { doneCallback(); }, mDoneCallback);
            notifier.detach();
        }

        yaget::metrics::TimeSpan mTimeSpan;
        yaget::Database& mDatabase;
        DoneCallback mDoneCallback;
        yaget::mt::JobPool mRequestPool;
        std::atomic_size_t mCounter{ 0 };

        // accumulate all file info while iterating over sections
        std::mutex mSectionMutex;
        using SectionKey = std::pair<std::string, std::string>;
        std::map<SectionKey, yaget::Strings> mSections;
    };
    
    //--------------------------------------------------------------------------------------------------
    std::string ResolveDatabaseName(const std::string& userFileName, bool reset)
    {
        std::string fileName = yaget::util::ExpendEnv(userFileName, nullptr);
        if (fileName.empty())
        {
            fileName = yaget::util::ExpendEnv(DatabasePathName, nullptr);
        }

        if (reset)
        {
            std::error_code ec;
            std::uintmax_t result = fs::remove(fs::path(fileName), ec);
            if (result == static_cast<std::uintmax_t>(-1))
            {
                std::string message = fmt::format("VTS", "Delete database file '{}' from disk failed with error: '{}: {}'.", fileName, ec.value(), ec.message());
                YAGET_UTIL_THROW("VTS", message.c_str());
            }
        }

        return fileName;
    }

    // Handles safely to decrement tag counter in event of exception or early return.
    class TagCounterKeeper
    {
    public:
        TagCounterKeeper(std::atomic_size_t* tagsCounter, const yaget::io::Tag& requestedTag)
            : mTagsCounter(tagsCounter)
            , mRequestedTag(requestedTag)
        {}

        ~TagCounterKeeper()
        {
            if (mTagsCounter)
            {
                YAGET_ASSERT(mTagsCounter->load() > 0, "Tags Counter value must be larger then 0 for Tag: '%s'.", mRequestedTag.mVTSName.c_str());
                (*mTagsCounter)--;
            }
        }

    private:
        std::atomic_size_t* mTagsCounter;
        const yaget::io::Tag& mRequestedTag;
    };

} // namespace


yaget::io::VirtualTransportSystem::Section::Section()
    : Name(mName)
    , Filter(mFilter)
    , Match(mMatch)
{
}


yaget::io::VirtualTransportSystem::Section::Section(const std::string& pathName)
    : Section()
{
    Strings tokens = conv::Split(pathName, "@");
    if (!tokens.empty() && tokens[0].size() > 1)
    {
        mName = tokens[0];
        const char operation = mName[0];

        // check to see if first character is user option for match filter
        const FilterMatch kMatches[] = { FilterMatch::Exact, FilterMatch::Override };
        const std::string kOperations = "=>";
        size_t index = kOperations.find_first_of(operation);

        if (tokens.size() > 1)
        {
            mFilter = tokens[1];

            // we only allow user operation if there is a Filter
            if (index != std::string::npos)
            {
                mMatch = kMatches[index];
            }
        }

        if (index != std::string::npos)
        {
            mName.erase(0, 1);

            if (mFilter.empty())
            {
                mMatch = FilterMatch::Like;
            }
        }
    }
}


yaget::io::VirtualTransportSystem::Section::Section(const io::Tag& tag)
    : Section(tag.mSectionName + "@" + tag.mName)
{
}


yaget::io::VirtualTransportSystem::Section::Section(const Section& source)
    : Section()
{
    *this = source;
}


yaget::io::VirtualTransportSystem::Section& yaget::io::VirtualTransportSystem::Section::operator=(const Section& source)
{
    mName = source.mName;
    mFilter = source.mFilter;
    mMatch = source.mMatch;
    return *this;
}


bool yaget::io::VirtualTransportSystem::Section::operator==(const Section& other) const
{
    return Name == other.Name && Filter == other.Filter && Match == other.Match;
}


std::string yaget::io::VirtualTransportSystem::Section::ToString() const
{
    if (mName.empty())
    {
        return "";
    }

    std::string result = FilterMatchCh[static_cast<int>(mMatch)] + mName;
    result += !mFilter.empty() ? "@" + mFilter : "";

    return result;
}


yaget::io::VirtualTransportSystem::VirtualTransportSystem(dev::Configuration::Init::VTSConfigList configList, VirtualTransportSystem::DoneCallback doneCallback, const AssetResolvers& assetResolvers, const std::string& fileName, RuntimeMode reset)
    : mRuntimeMode(RuntimeMode::Optimum)
    , mDoneCallback(doneCallback)
    , mRequestPool("vts.Request", dev::CurrentConfiguration().mDebug.mThreads.VTS)
    , mAssetResolvers(assetResolvers)
    , mDatabase(ResolveDatabaseName(fileName, reset == RuntimeMode::Reset), vtsSchema, YAGET_VTS_VERSION)
    , mSectionEntriesCollector(std::make_shared<SectionEntriesCollector>(configList, mDatabase, [this]() { onEntriesCollected(); }))
    , mBlobLoader([this](auto&&... params) { onErrorBlobLoader(params...); })
{
}


yaget::io::VirtualTransportSystem::VirtualTransportSystem(RuntimeMode runtimeMode, const std::string& fileName)
    : mRuntimeMode(runtimeMode)
    , mRequestPool("vts.Request", 1)
    , mDatabase(ResolveDatabaseName(fileName, false), vtsSchema, YAGET_VTS_VERSION)
    , mBlobLoader([this](auto&&... params) { onErrorBlobLoader(params...); })
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
        using DirtyRow = SQLite::Row<Guid /*guid*/, std::string /*VTS*/, std::string /*Section*/>;
        std::vector<DirtyRow> dirtyBlobs = mDatabase.DB().GetRows<DirtyRow>("SELECT Tags.Guid, Tags.VTS, Tags.Section FROM Tags INNER JOIN DirtyTags ON Tags.Guid=DirtyTags.Guid;");

        for (auto it : dirtyBlobs)
        {
            io::Tag tag;
            tag.mGuid = it.Result;
            auto asset = FindAsset(tag);
            YAGET_ASSERT(asset, "Did not find asset: '%s' in collection while trying to save dirty blob.", it.Result1.c_str());

            std::string fileName = util::ExpendEnv(it.Result1, nullptr);
            if (!fs::path(fileName).has_extension())
            {
                std::string command = fmt::format("SELECT Filters FROM 'Sections' WHERE Name = '{}';", it.Result2);
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
            std::string command = fmt::format("SELECT Sections.Converters FROM Sections INNER JOIN Tags ON Tags.Guid = '{}' AND Sections.Name = Tags.Section;", requestedTag.mGuid.str());
            std::string converterType;

            if (DatabaseHandle dHandle = LockDatabaseAccess())
            {
                converterType = GetCell<std::string>(dHandle->DB(), command);
            }

            auto it = mAssetResolvers.find(converterType);
            YAGET_ASSERT(it != mAssetResolvers.end(), "Asset Resolvers does not have entry for: '%s' for tag: '%s'.", converterType.c_str(), requestedTag.mVTSName.c_str());

            if (auto newAsset = it->second(dataBuffer, requestedTag, *this))
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
        std::string message = fmt::format("Blob '{}' did not get converted. '{}'.", requestedTag.mVTSName.c_str(), e.what());
        YLOG_ERROR("VTS", message.c_str());
    }
}


//--------------------------------------------------------------------------------------------------
std::shared_ptr<yaget::io::Asset> yaget::io::VirtualTransportSystem::AddAsset(std::shared_ptr<Asset> asset)
{
    // due to mt nature of vts, the query for asset returning null and then later creating that new asset
    // can be interrupted in the middle and insert that asset, making "first" creation duplicate
    std::unique_lock<std::mutex> locker(mMutexAssets);
    return AddAssetNonMT(asset);
}


//--------------------------------------------------------------------------------------------------
std::shared_ptr<yaget::io::Asset> yaget::io::VirtualTransportSystem::AddAssetNonMT(std::shared_ptr<yaget::io::Asset> asset)
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
        mCashedAssets.erase(tag);
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
    auto it = mCashedAssets.find(tag);
    if (it != mCashedAssets.end())
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
void yaget::io::VirtualTransportSystem::AddOverride(std::shared_ptr<io::Asset> asset)
{
    std::unique_lock<std::mutex> locker(mMutexAssets);

    YAGET_ASSERT(FindAssetNonMT(asset->mTag), "Trying to add override asset: '%s' that does not exist.", asset->mTag.mVTSName.c_str());

    auto result = mCashedAssets.insert(std::make_pair(asset->mTag, asset));
    YAGET_ASSERT(result.second, "Asset: '%s' already exists in cashed collection.", asset->mTag.mVTSName.c_str());
}


//--------------------------------------------------------------------------------------------------
void yaget::io::VirtualTransportSystem::AttachTransientBlob(const std::vector<std::shared_ptr<io::Asset>>& assets)
{
    std::vector<std::shared_ptr<io::Asset>> attachedAssets;
    if (DatabaseHandle databaseHandle = LockDatabaseAccess())
    {
        SQLite& database = databaseHandle->DB();
        yaget::db::Transaction transaction(database);

        for (const auto& it : assets)
        {
            const io::Tag& tag = it->mTag;
            io::db::TagRecord tagRecord(tag.mGuid, tag.mName, tag.mVTSName, tag.mSectionName);

            if (!database.ExecuteStatement("TagInsert", "Tags", tagRecord, SQLite::Behaviour::NoTimeStamp, SQLite::Behaviour::Insert))
            {
                transaction.Rollback();

                std::string message = fmt::format("Attaching blob '{}' to Section: '{}' as VTS: '{}' failed. {}.", tag.mName, tag.mSectionName, tag.mVTSName, ParseErrors(database));
                YLOG_ERROR("VTS", message.c_str());

                return;
            }

            attachedAssets.push_back(it);
        }
    }

    //std::unique_lock<std::mutex> locker(mMutexAssets);
    for (const auto& asset : attachedAssets)
    {
        //(void)AddAssetNonMT(asset);
        (void)AddAsset(asset);
    }
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

        // lock asset list for any changes
        std::unique_lock<std::mutex> locker(mMutexAssets);
        auto assetData = FindAssetNonMT(tag);
        if (!assetData && request == Request::Add)
        {
            io::db::TagRecord tagRecord(tag.mGuid, tag.mName, tag.mVTSName, tag.mSectionName);
            if (!database.ExecuteStatement("TagInsert", "Tags", tagRecord, SQLite::Behaviour::NoTimeStamp, SQLite::Behaviour::Insert) ||
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
                    std::string vtsName = *it + "/" + section.Filter;
                    query = io::db::TagRecordQuery(Section(operation + section.Name + "@" + vtsName));
                    std::vector<io::Tag> nextResults = dHandle->DB().GetRows<io::Tag, io::db::TagRecord>(query, [](const io::db::TagRecord& record) { return io::Tag{ record.Result1, record.Result, record.Result2, record.Result3 }; });

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
    using TargetSection = SQLite::Row<std::string /*Name*/, Strings /*Path*/, Strings /*Filters*/, std::string /*Converters*/, bool /*ReadOnly*/>;

    io::Tag newTag;
    std::string command = fmt::format("SELECT Name, Path, Filters, Converters, ReadOnly FROM Sections WHERE Name = '{}'", section.Name);

    if (DatabaseHandle databaseHandle = LockDatabaseAccess())
    {
        SQLite& database = databaseHandle->DB();

        if (TargetSection targetSection = database.GetRow<TargetSection>(command); targetSection.bValid)
        {
            fs::path filePath = *targetSection.Result1.begin() + "/" + section.Filter;

            newTag.mName = filePath.filename().stem().string();
            newTag.mVTSName = filePath.generic_string();
            newTag.mSectionName = section.Name;

            command = fmt::format("SELECT Guid FROM Deleted WHERE VTS = '{}';", newTag.mVTSName);
            Guid recoveredGuid = GetCell<Guid>(database, command);
            if (recoveredGuid.IsValid())
            {
                command = fmt::format("DELETE FROM Deleted WHERE Guid = '{}';", recoveredGuid.str());
                if (!database.ExecuteStatement(command.c_str(), nullptr))
                {
                    YLOG_ERROR("VTS", "Did not deleted: '%s' from Deleted table. %s", newTag.mVTSName.c_str(), ParseErrors(database).c_str());
                }
                else
                {
                    newTag.mGuid = recoveredGuid;
                }
            }
        }
        else
        {
            YLOG_ERROR("VTS", "Did not get Target Section from section: '%s'.", section.ToString().c_str());
            return {};
        }
    }

    newTag.mGuid = newTag.mGuid.IsValid() ? newTag.mGuid : NewGuid();
    return newTag;
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
