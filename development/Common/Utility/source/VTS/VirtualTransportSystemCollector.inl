//VirtualTransportSystemCollector.inl

#include "HashUtilities.h"

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
                if (!section.ReadOnly && section.Path.size() > 1)
                {
                    YLOG_ERROR("VTS", "Section '%s' is marked writeable but it has '%d' [%s] path entries. Only 1 is allowed, skipping.", section.Name.c_str(), section.Path.size(), yaget::conv::Combine(section.Path, "],[").c_str());
                    continue;;
                }

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
            : mTimeSpan(yaget::meta::pointer_cast(this), "VTS Entries Collector", YAGET_METRICS_CHANNEL_FILE_LINE)
            , mDatabase(database)
            , mDoneCallback(std::move(doneCallback))
            , mRequestPool("INDX", yaget::dev::CurrentConfiguration().mDebug.mThreads.VTSSections)
        {
            using namespace yaget;
            using VTS = dev::Configuration::Init;
            using SectionRecord = std::tuple<std::string /*Name*/, Strings /*Path*/, Strings /*Filters*/, std::string /*Converters*/, bool /*ReadOnly*/, bool /*Recursive*/>;

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
                    YAGET_UTIL_THROW("VTS", message);
                }

                mDatabase.Log("SESSION_START", "VTS Started");

                if (int numDirtyBlobs = GetCell<int>(mDatabase.DB(), "SELECT COUNT(*) FROM 'DirtyTags';"))
                {
                    transaction.Rollback();
                    std::string message = fmt::format("There are '{}' blobs left in DirtyTags table.", numDirtyBlobs);
                    YAGET_UTIL_THROW("VTS", message);
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
                    YAGET_UTIL_THROW("VTS", message);
                }

                //VTS::VTSConfigList
                mCounter = std::accumulate(configList.begin(), configList.end(), static_cast<size_t>(0), [](const auto& runningTotal , const auto& vtsConfig)
                {
                    return runningTotal + vtsConfig.Path.size();
                });

                std::string command = fmt::format("SELECT Name, Path, Filters, Converters, ReadOnly, Recursive FROM Sections ORDER BY Name;");
                VTS::VTSConfigList sectionRecords = mDatabase.DB().GetRowsTuple<VTS::VTS, SectionRecord, VTS::VTSConfigList>(command, [](const SectionRecord& record)
                    {
                        return VTS::VTS{ std::get<0>(record), std::get<1>(record), std::get<2>(record), std::get<3>(record), std::get<4>(record), std::get<5>(record) };
                    });

                std::set_difference(configList.begin(), configList.end(), sectionRecords.begin(), sectionRecords.end(), std::inserter(newSection, newSection.end()));
                std::set_difference(sectionRecords.begin(), sectionRecords.end(), configList.begin(), configList.end(), std::inserter(deletedSections, deletedSections.end()));
                std::set_intersection(configList.begin(), configList.end(), sectionRecords.begin(), sectionRecords.end(), std::inserter(changedSections, changedSections.end()));

                // and proceed with making db sections match current state of the disk
                for (const auto& it : newSection)
                {
                    SectionRecord section(it.Name, it.Path, it.Filters, it.Converters, it.ReadOnly, it.Recursive);
                    if (!mDatabase.DB().ExecuteStatementTuple("SectionInsert", "Sections", section, { "Name", "Path", "Filters", "Converters", "ReadOnly", "Recursive" }, SQLite::Behaviour::Insert))
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
                        if (!mDatabase.DB().ExecuteStatementTuple("SectionInsert", "Sections", section, { "Name", "Path", "Filters", "Converters", "ReadOnly", "Recursive" }, SQLite::Behaviour::Update))
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

            metrics::Channel channel("UpdateDatabase", YAGET_METRICS_CHANNEL_FILE_LINE);

            size_t numNewTags = 0, numDeletedTags = 0;

            // any errors during updates to tag system, will result in full rollback
            db::Transaction transaction(mDatabase.DB());
            for (const auto& section : mSections)
            {
                using TagRecordTuple = std::tuple<Guid /*Guid*/, std::string /*Name*/, std::string /*VTS*/, std::string /*Section*/>;

                const std::string& nameSection = section.first.first;
                const std::string& namePath = section.first.second;
                const Strings& tagFiles = section.second;

                std::string command = io::db::TagRecordQuery(io::VirtualTransportSystem::Section(nameSection + "@" + namePath));
                Strings tagRecords = mDatabase.DB().GetRowsTuple<std::string, TagRecordTuple, Strings>(command, [](const TagRecordTuple& record) { return std::get<2>(record); });

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
                    std::vector<TagRecordTuple> deletedBlobs;

                    if (deletedRowCount < 1000)
                    {
                        // load entire table into memory, since it's a small one, otherwise just query db for each one
                        command = fmt::format("SELECT Guid, Name, VTS, Section FROM Deleted;");
                        deletedBlobs = mDatabase.DB().GetRowsTuple<TagRecordTuple>(command);
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
                                        return std::get<2>(param) == vtsName;
                                    });

                                if (it != deletedBlobs.end())
                                {
                                    recoveredGuid = std::get<3>(*it);
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

                        using TagRecordTuple = std::tuple<Guid /*Guid*/, std::string /*Name*/, std::string /*VTS*/, std::string /*Section*/>;

                        TagRecordTuple tag(recoveredGuid, fs::path(vtsName).filename().stem().generic_string(), vtsName, nameSection);
                        if (!mDatabase.DB().ExecuteStatementTuple("TagInsert", "Tags", tag, { "Guid", "Name", "VTS", "Section" }, SQLite::Behaviour::Insert))
                        {
                            transaction.Rollback();
                            std::string message = fmt::format("TagInsert: '{}' for vts section: {} failed. {}.", vtsName, nameSection, ParseErrors(mDatabase.DB()));
                            YAGET_UTIL_THROW("VTS", message.c_str());
                        }

                        numNewTags++;
                    }

                    for (const auto& it : deletedTags)
                    {
                        using TagData = std::tuple<Guid /*Guid*/, std::string /*Name*/, std::string /*VTS*/, std::string /*Section*/>;
                        bool result = true;

                        command = io::db::TagRecordQuery(io::VirtualTransportSystem::Section(nameSection + "@" + it));
                        TagData existingTag = mDatabase.DB().GetRowTuple<TagData>(command, &result);
                        if (!result)
                        {
                            transaction.Rollback();
                            std::string message = fmt::format("VTS: '{}' does not exist in Tags table. {}.", it, ParseErrors(mDatabase.DB()));
                            YAGET_UTIL_THROW("VTS", message.c_str());
                        }

                        command = fmt::format("DELETE FROM Tags WHERE Guid = '{}';", std::get<0>(existingTag).str());
                        if (!mDatabase.DB().ExecuteStatement(command.c_str(), nullptr))
                        {
                            transaction.Rollback();
                            std::string message = fmt::format("Did not delete tag: '{}' with section: {}. {}.", it, nameSection, ParseErrors(mDatabase.DB()));
                            YAGET_UTIL_THROW("VTS", message.c_str());
                        }

                        if (!mDatabase.DB().ExecuteStatementTuple("DeletedInsert", "Deleted", existingTag, { "Guid", "Name", "VTS", "Section" }, SQLite::Behaviour::Update))
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
            std::thread notifier = std::thread([](DoneCallback doneCallback)
                {
                    metrics::MarkStartThread(platform::CurrentThreadId(), "INDXDONE");

                    doneCallback();
                }, mDoneCallback);

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
            if (result == static_cast<std::uintmax_t>(-1) || result == 0)
            {
                std::string message = fmt::format("Delete database file '{}' from disk failed with error: '{}: {}'.", fileName, ec.value(), ec.message());
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
                (void)(*mTagsCounter)--;
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


inline yaget::io::VirtualTransportSystem::Section::Section(const io::Tag& tag)
    : Section(tag.mSectionName + "@" + tag.mName)
{
}


inline yaget::io::VirtualTransportSystem::Section::Section(const Section& source)
    : Section()
{
    *this = source;
}


inline yaget::io::VirtualTransportSystem::Section& yaget::io::VirtualTransportSystem::Section::operator=(const Section& source)
{
    mName = source.mName;
    mFilter = source.mFilter;
    mMatch = source.mMatch;
    return *this;
}


inline bool yaget::io::VirtualTransportSystem::Section::operator==(const Section& other) const
{
    return Name == other.Name && Filter == other.Filter && Match == other.Match;
}


inline std::string yaget::io::VirtualTransportSystem::Section::ToString() const
{
    if (mName.empty())
    {
        return "";
    }

    std::string result = FilterMatchCh[static_cast<int>(mMatch)] + mName;
    result += !mFilter.empty() ? "@" + mFilter : "";

    return result;
}
