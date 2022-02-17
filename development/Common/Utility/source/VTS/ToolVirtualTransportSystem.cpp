#include "VTS/ToolVirtualTransportSystem.h"
#include "StringHelpers.h"
#include "App/FileUtilities.h"
#include "App/AppUtilities.h"

#include <filesystem>

#include "VTS/ResolvedAssets.h"
namespace fs = std::filesystem;


yaget::io::tool::VirtualTransportSystem::VirtualTransportSystem(dev::Configuration::Init::VTSConfigList configList, const AssetResolvers& assetResolvers, const std::string& fileName, RuntimeMode reset)
    : io::VirtualTransportSystem(configList, [this]() { mKeepWaiting = false; }, assetResolvers, fileName, reset)
{
    platform::Sleep([this]() { return mKeepWaiting; });

    // so we don't hit db unnecessary (SectionRecord for log tags), and also this code might go away.
    if (YLOG_IS_TAG_VISIBLE("VTS"))
    {
        using SectionRecord = std::tuple<std::string /*Name*/, std::string /*Path*/, std::string /*Filters*/, bool /*ReadOnly*/, bool /*Recursive*/>;
        const std::string command = fmt::format("SELECT Name, Path, Filters, ReadOnly, Recursive FROM Sections;");

        DatabaseHandle dbLocker = LockDatabaseAccess();
        std::vector<SectionRecord> sections = dbLocker->DB().GetRowsTuple<SectionRecord>(command);
        std::string logMessage = "VTS Sections:\n";
        auto numSections = sections.size();
        for (const auto& section : sections)
        {
            --numSections;

            logMessage += fmt::format("Section: '{}', Path: '[{}]', Filters: '[{}]', ReadOnly: '{}', Recursive: '{}'\n", std::get<0>(section), std::get<1>(section), std::get<2>(section), pp::log(std::get<3>(section)), pp::log(std::get<4>(section)));
            auto resolvedPaths = conv::Split(std::get<1>(section), ",");
            auto numPaths = resolvedPaths.size();
            for (const auto& it : resolvedPaths)
            {
                logMessage += fmt::format("    Expended Path: '{}'", util::ExpendEnv(it, nullptr));

                if (--numPaths || numSections)
                {
                    logMessage += "\n";
                }
            }
        }

        YLOG_INFO("VTS", logMessage.c_str());
    }
}


//YAGET_COMPILE_SUPRESS_START(4189, "'': local variable is initialized but not referenced")

bool yaget::io::tool::VirtualTransportSystem::AttachBlob(const std::vector<std::shared_ptr<io::Asset>>& assets)
{
    std::vector<std::shared_ptr<io::Asset>> attachedAssets;
    if (DatabaseHandle databaseHandle = LockDatabaseAccess())
    {
        SQLite& database = databaseHandle->DB();

        using SectionRecord = std::tuple<std::string /*Name*/, Strings /*Path*/, std::string /*Filters*/, bool /*ReadOnly*/, bool /*Recursive*/>;
        const std::string command = fmt::format("SELECT Name, Path, Filters, ReadOnly, Recursive FROM Sections;");
        std::vector<SectionRecord> sections = database.GetRowsTuple<SectionRecord>(command);

        yaget::db::Transaction transaction(database);
        using TagRecordTuple = std::tuple<Guid /*Guid*/, std::string /*Name*/, std::string /*VTS*/, std::string /*Section*/>;

        std::vector<std::pair<io::Tag, Section>> duplicateFiles;

        for (const auto& it : assets)
        {
            const io::Tag& tag = it->mTag;
            TagRecordTuple tagRecord(tag.mGuid, tag.mName, tag.mVTSName, tag.mSectionName);
            std::string dirtyCommand = fmt::format("INSERT INTO 'DirtyTags' VALUES('{}');", tag.mGuid.str());

            if (!database.ExecuteStatementTuple("TagInsert", "Tags", tagRecord, { "Guid", "Name", "VTS", "Section" }, SQLite::Behaviour::Insert) ||
                !database.ExecuteStatement(dirtyCommand, nullptr))
            {
                transaction.Rollback();

                std::string message = fmt::format("Attaching blob '{}' to Section: '{}' as VTS: '{}' failed. {}.", tag.mName, tag.mSectionName, tag.mVTSName, ParseErrors(database));
                YLOG_WARNING("VTS", message.c_str());

                return false;
            }

            // now we also need to find same path for any other sections, and if it matches
            // we need to add entry to Tags folder under that Section which matched our path
            fs::path newBlobFilePath = tag.ResolveVTS();
            const auto& newBlobPath = newBlobFilePath.parent_path();

            // iterate over all sections
            for (const auto& s : sections)
            {
                // for each Path, expend and compare this path to newBlobPath, but skip tag.mSectionName
                if (std::get<0>(s) == tag.mSectionName)
                {
                    continue;
                }

                for (const auto& p : std::get<1>(s))
                {
                    fs::path sectionPath = util::ExpendEnv(p, nullptr);
                    if (newBlobPath == sectionPath)
                    {
                        // this blob also need entry under this section
                        duplicateFiles.emplace_back(std::make_pair(tag, Section(std::get<0>(s) + "@" + p)));

                        // we need to actually duplicate tag for this asset
                        io::Tag dupTag = tag;
                        dupTag.mGuid = NewGuid();
                        dupTag.mSectionName = std::get<0>(s);

                        const std::string dupCommand = fmt::format("SELECT Sections.Converters FROM Sections INNER JOIN Tags ON Tags.Guid = '{}' AND Sections.Name = Tags.Section;", tag.mGuid.str());
                        auto converterType = GetCell<std::string>(database, dupCommand);
                        if (auto converter = FindAssetConverter(converterType))
                        {
                            if (auto newAsset = converter(it->mBuffer, dupTag, *this))
                            {
                                TagRecordTuple newTagRecord(dupTag.mGuid, dupTag.mName, dupTag.mVTSName, dupTag.mSectionName);

                                if (!database.ExecuteStatementTuple("TagInsert", "Tags", newTagRecord, { "Guid", "Name", "VTS", "Section" }, SQLite::Behaviour::Insert))
                                {
                                    transaction.Rollback();

                                    std::string message = fmt::format("Attaching blob '{}' to Section: '{}' as VTS: '{}' failed. {}.", dupTag.mName, dupTag.mSectionName, dupTag.mVTSName, ParseErrors(database));
                                    YLOG_WARNING("VTS", message.c_str());

                                    return false;
                                }

                                attachedAssets.push_back(newAsset);
                            }
                        }
                    }
                }
            }

            attachedAssets.push_back(it);
        }
    }


    for (const auto& it : attachedAssets)
    {
        AddAsset(it);
    }

    return true;
}
//YAGET_COMPILE_SUPRESS_END


yaget::io::Tag yaget::io::tool::VirtualTransportSystem::CopyTag(const io::Tag& sourceTag, const Section& toSection, Options flat) const
{
    using SourceSection = std::tuple<std::string /*Name*/, Strings /*Path*/>;
    using TargetSection = std::tuple<std::string /*Name*/, Strings /*Path*/, Strings /*Filters*/, std::string /*Converters*/, bool /*ReadOnly*/>;

    std::string commandSourceSection = fmt::format("SELECT Sections.Name, Sections.Path FROM Tags INNER JOIN Sections ON Tags.Guid = '{}' AND Sections.Name = Tags.Section;", sourceTag.mGuid.str());
    std::string commandTargetSection = fmt::format("SELECT Name, Path, Filters, Converters, ReadOnly FROM Sections WHERE Name = '{}'", toSection.Name);

    SourceSection sourceSection{};
    TargetSection targetSection{};
    if (DatabaseHandle databaseHandle = LockDatabaseAccess())
    {
        SQLite& database = databaseHandle->DB();

        bool result = true;
        sourceSection = database.GetRowTuple<SourceSection>(commandSourceSection, &result);
        if (!result)
        {
            YLOG_ERROR("VTS", "Did not find source '%s' section. %s", sourceTag.mSectionName.c_str(), ParseErrors(database).c_str());
            return {};
        }

        targetSection = database.GetRowTuple<TargetSection>(commandTargetSection, &result);
        if (!result)
        {
            YLOG_ERROR("VTS", "Did not find target '%s' section. %s", toSection.Name.c_str(), ParseErrors(database).c_str());
            return {};
        }
    }

    io::Tag newTag{};
    std::string targetExtension = fs::path(*std::get<2>(targetSection).begin()).extension().string();

    if (!std::get<4>(targetSection))
    {
        for (const auto& path : std::get<1>(sourceSection))
        {
            if (sourceTag.mVTSName.compare(0, path.size(), path) == 0)
            {
                std::string suffixFile = flat == Options::Flat ? fs::path(sourceTag.mVTSName).filename().string() : sourceTag.mVTSName.substr(path.size() + 1);
                suffixFile = fs::path(suffixFile).replace_extension(targetExtension).generic_string();
                Section testSection(toSection.Name + "@" + toSection.Filter + "/" + suffixFile);
                newTag = GenerateTag(testSection);

                break;
            }
        }
    }
    else
    {
        YLOG_WARNING("VTS", "Requested tag copy: '%s' to section '%s' can not be done due to read only flag.", sourceTag.mVTSName.c_str(), toSection.Name.c_str());
    }

    return newTag;
}


//--------------------------------------------------------------------------------------------------
bool yaget::io::tool::VirtualTransportSystem::DeleteBlob(const Sections& sections)
{
    std::vector<io::Tag> tags = GetTags(sections);

    if (DatabaseHandle databaseHandle = LockDatabaseAccess())
    {
        SQLite& database = databaseHandle->DB();
        yaget::db::Transaction transaction(database);
        using TagRecordTuple = std::tuple<Guid /*Guid*/, std::string /*Name*/, std::string /*VTS*/, std::string /*Section*/>;

        for (const auto& tag : tags)
        {
            TagRecordTuple deletedTag(tag.mGuid, tag.mName, tag.mVTSName, tag.mSectionName);

            // first, let's remove it from Tags table, then from this.collection and then from disk
            const auto& dirtyCommand = fmt::format("DELETE FROM DirtyTags WHERE Guid = '{}'", tag.mGuid.str());
            const auto& deleteCommand = fmt::format("DELETE FROM Tags WHERE Guid = '{}'", tag.mGuid.str());

            if (database.ExecuteStatement(dirtyCommand, nullptr) &&
                database.ExecuteStatement(deleteCommand, nullptr) &&
                database.ExecuteStatementTuple("DeletedInsert", "Deleted", deletedTag, { "Guid", "Name", "VTS", "Section" }, SQLite::Behaviour::Update))
            {
                std::string fileName = util::ExpendEnv(tag.mVTSName, nullptr);
                std::error_code ec;
                std::uintmax_t result = fs::remove(fs::path(fileName), ec);
                if (result == static_cast<std::uintmax_t>(-1))
                {
                    transaction.Rollback();

                    YLOG_ERROR("VTS", "Delete blob '%s' from disk failed with error: '%d: %s'.", fileName.c_str(), ec.value(), ec.message().c_str());
                    return false;
                }
            }
            else
            {
                transaction.Rollback();

                YLOG_ERROR("VTS", "Delete blob '%s' failed with db error: %s.", tag.mVTSName.c_str(), ParseErrors(database).c_str());
                return false;
            }
        }

        // delete folder if it's empty
        for (const auto& tag : tags)
        {
            std::string folder = util::ExpendEnv(tag.mVTSName, nullptr);
            if (file::GetFileNames(folder, true, [](const std::string& /*fileName*/) { return true; }).empty())
            {
                std::error_code ec;
                std::uintmax_t result = fs::remove_all(fs::path(folder), ec);
                if (result == static_cast<std::uintmax_t>(-1))
                {
                    transaction.Rollback();

                    YLOG_ERROR("VTS", "Delete folder '%s' from disk failed with error: '%d: %s'.", folder.c_str(), ec.value(), ec.message().c_str());
                    return false;
                }
            }
        }

        for (const auto& tag : tags)
        {
            RemoveAsset(tag);
        }
    }

    return true;
}

