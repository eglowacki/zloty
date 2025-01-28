#include "HashUtilities.h"

#include "Items/ItemsDirector.h"
#include "App/AppUtilities.h"
#include "Meta/CompilerAlgo.h"
#include "Exception/Exception.h"

#include <filesystem>

#include "Core/ErrorHandlers.h"

namespace fs = std::filesystem;

#define YAGET_DIRECTOR_VERSION 5


namespace
{
    // NOTES:
    // UPDATE Products SET Price = Price + 50
    // UPDATE Products SET Price = Price + 50 WHERE ProductID = 1
    // UPDATE{ Table } SET{ Column } = { Column } + {Value} WHERE{ Condition }
    //
    //{Table} - table name
    //{Column} - column name
    //{Value} - a number by which column's value should be increased or decreased
    //{Condition} - some condition if any
    //
    std::vector<std::string> itemsSchema =
    {
        #include "Items/ItemsSchema.sqlite"
        "CREATE TABLE 'VersionTables' ('Id' INTEGER NOT NULL DEFAULT 1, PRIMARY KEY('Id')) WITHOUT ROWID;",
        "CREATE TABLE 'Stages' ('Id' INTEGER NOT NULL UNIQUE, 'Name' TEXT NOT NULL UNIQUE, PRIMARY KEY('Id' AUTOINCREMENT));",
        "CREATE TABLE 'StageItems' ('ItemId' INTEGER NOT NULL, 'StageId' INTEGER NOT NULL, FOREIGN KEY('StageId') REFERENCES 'Stages'('Id'), UNIQUE(ItemId, StageId) ON CONFLICT REPLACE);"
    };


#if 0
    INSERT INTO 'Stages' ('Name') VALUES('Level 1');
    INSERT INTO 'Stages' ('Name') VALUES('Main Menu');

    INSERT INTO 'StageItems' ('StageId', 'ItemId') VALUES(1, 417);

    SELECT * FROM 'Stages';
#endif

    std::string ResolveDatabaseName(const std::string& userFileName, bool reset)
    {
        std::string fileName = yaget::util::ExpendEnv(userFileName, nullptr);

        if (reset)
        {
            std::error_code ec;
            std::uintmax_t result = fs::remove(fs::path(fileName), ec);
            if (result == static_cast<std::uintmax_t>(-1))
            {
                const std::string message = fmt::format("DIRE", "Delete database file '{}' from disk failed with error: '{}: {}'.", fileName, ec.value(), ec.message());
                yaget::error_handlers::Throw("DIRE", message);
            }
        }

        return fileName;
    }

    template<typename... Args>
    yaget::Strings CombineSchemas(Args&&... args)
    {
        using namespace yaget;

        Strings result;

        using StringsPack = std::tuple<Args...>;
        StringsPack strings(args...);

        meta::for_each(strings, [&result](const auto& schemaText)
        {
            result.insert(result.begin(), schemaText.begin(), schemaText.end());
        });

        return result;
    }

} // namespace


yaget::items::Director::Director(const std::string& name, const Strings& additionalSchema, const Strings& loadout, int64_t expectedVersion, RuntimeMode runtimeMode)
    : mDatabase(ResolveDatabaseName(name, runtimeMode == RuntimeMode::Reset), CombineSchemas(additionalSchema, Strings{fmt::format("INSERT INTO VersionTables(Id) VALUES('{}');", expectedVersion)}, itemsSchema), YAGET_DIRECTOR_VERSION)
    , mIdGameCache([this]() { return GetNextBatch(); })
{
    const auto tablesVersion = GetCell<int64_t>(mDatabase.DB(), "SELECT Id FROM VersionTables;");
    error_handlers::ThrowOnCheck((expectedVersion == Database::NonVersioned || (expectedVersion != Database::NonVersioned && tablesVersion == expectedVersion)),
                                 fmt::format("Director Database '{}' has mismatched version. Expected: '{}', result: '{}'.", 
                                             yaget::util::ExpendEnv(name, nullptr).c_str(),
                                             expectedVersion, 
                                             tablesVersion));

    YLOG_INFO("DIRE", "Items Director initialized '%s'.", util::ExpendEnv(name, nullptr).c_str());

    CacheStageNames();

    if (!loadout.empty())
    {
        int64_t loadoutVersion = 0;
        Strings sqlLoadout;
        comp::Id_t itemId = comp::INVALID_ID;

        for (const auto& command : loadout)
        {
            if (command == comp::db::NewItem_Token)
            {
                itemId = idspace::get_persistent(mIdGameCache);
                continue;
            }

            error_handlers::ThrowOnCheck(itemId != comp::INVALID_ID, fmt::format("ItemId in this scope is invalid. Is '{}' token as a first line in loadout file missing?", comp::db::NewItem_Token));

            conv::hash_combine(loadoutVersion, command);
            sqlLoadout.emplace_back(fmt::vformat(command, fmt::make_format_args(itemId)));
        }

        const char* hashesTable = "Hashes";
        const char* hashesKey = "loadout.start";

        SQLite& database = mDatabase.DB();
        // before we update current loadout, let's check version
        const auto version = GetCell<int64_t>(database, fmt::format("SELECT Value FROM {} WHERE Key = '{}';", hashesTable, hashesKey));
        if (version == 0)
        {
            db::Transaction transaction(database);

            for (const auto& command : sqlLoadout)
            {
                if (!database.ExecuteStatement(command, nullptr))
                {
                    transaction.Rollback();
                    error_handlers::Throw("DIRE", fmt::format("Could not execute sql query '{}'. {}.", command, ParseErrors(database)));
                }
            }

            std::string sqCommand = fmt::format("INSERT OR REPLACE INTO '{}' VALUES('{}', {});", hashesTable, hashesKey, loadoutVersion);
            if (!database.ExecuteStatement(sqCommand, nullptr))
            {
                transaction.Rollback();
                error_handlers::Throw("DIRE", fmt::format("Could not update {} '{}' sql query '{}'.", hashesTable, sqCommand, ParseErrors(database)));
            }

            YLOG_INFO("DIRE", "Items Director's loadout is done, added: '%d' items.", sqlLoadout.size());
        }
        else if (version == loadoutVersion)
        {
            YLOG_INFO("DIRE", "Items Director's current loadout is same as incomming, safely ignoring.");
        }
        else
        {
            error_handlers::Throw("DIRE", fmt::format("Incoming loadout version: '{}' does not match one in db: '{}'.", loadoutVersion, version));
        }
    }
}


yaget::items::Director::~Director()
{
    YLOG_INFO("DIRE", "Items Director closed.");
}


yaget::comp::ItemIds yaget::items::Director::GetStageItems(const std::string& stageName) const
{
    yaget::comp::ItemIds result;

    if (DatabaseHandle databaseHandle = LockDatabaseAccess())
    {
        const SQLite& database = databaseHandle->DB();

        if (auto stageId = GetStageId(stageName); stageId != Director::InvalidStageId)
        {
            using itemId = std::tuple<comp::Id_t>;
            auto stageItemIds = database.GetRowsTuple<itemId>(fmt::format("SELECT ItemId FROM StageItems WHERE StageId = {};", stageId));

            int z = 0;
            z;
        }
    }

    return result;
}


void yaget::items::Director::AddStageItems(const std::string& stageName, const comp::ItemIds& ids)
{
    if (auto stageId =  GetStageId(stageName); stageId != Director::InvalidStageId)
    {
        if (DatabaseHandle databaseHandle = LockDatabaseAccess())
        {
            SQLite& database = databaseHandle->DB();

            for (const auto& id :ids)
            {
                const auto& command = fmt::format("INSERT INTO StageItems (ItemId, StageId) VALUES ({}, {});", id, stageId);

                if (!database.ExecuteStatement(command, nullptr))
                {
                    //transaction.Rollback();
                    const auto& message = fmt::format("Did not update Stage '{}' with add item '{}'. {}.", stageName, id, ParseErrors(database));
                    YLOG_ERROR("DIRE", message.c_str());
                }
            }
        }
    }
}


void yaget::items::Director::RemoveStageItems(const std::string& stageName, const comp::ItemIds& ids)
{
    if (auto stageId =  GetStageId(stageName); stageId != Director::InvalidStageId)
    {
        if (DatabaseHandle databaseHandle = LockDatabaseAccess())
        {
            SQLite& database = databaseHandle->DB();

            for (const auto& id :ids)
            {
                const auto& command = fmt::format("DELETE FROM StageItems WHERE ItemId = {} AND StageId = {};", id, stageId);

                if (!database.ExecuteStatement(command, nullptr))
                {
                    //transaction.Rollback();
                    const auto& message = fmt::format("Did not update Stage '{}' with remove item '{}'. {}.", stageName, id, ParseErrors(database));
                    YLOG_ERROR("DIRE", message.c_str());
                }
            }
        }
    }
}


yaget::items::IdBatch yaget::items::Director::GetNextBatch()
{
    if (DatabaseHandle databaseHandle = LockDatabaseAccess())
    {
        using Batch = std::tuple<comp::Id_t, uint64_t>;

        const std::string& nextBatchCommand = fmt::format("SELECT NextId, BatchSIze FROM IdCache WHERE Marker = {};", BatchIdMarker);
        const std::string& updateBatchCommand = fmt::format("UPDATE IdCache SET NextId = NextId + BatchSize WHERE Marker = {};", BatchIdMarker);

        SQLite& database = databaseHandle->DB();
        db::Transaction transaction(database);

        bool result = true;
        const auto nextBatch = database.GetRowTuple<Batch>(nextBatchCommand, &result);
        if (!result)
        {
            transaction.Rollback();
            error_handlers::Throw("DIRE", fmt::format("Did not get next Batch from db. %s.", ParseErrors(database)));
        }

        if (!database.ExecuteStatement(updateBatchCommand, nullptr))
        {
            transaction.Rollback();
            error_handlers::Throw("DIRE", fmt::format("Did not update next Batch into db. %s.", ParseErrors(database)));
        }

        return items::IdBatch{ std::get<0>(nextBatch), std::get<1>(nextBatch) };
    }

    error_handlers::Throw("DIRE", "Did not get locked db handle for Director's database");
    return{ 0, 0 };
}


void yaget::items::Director::CacheStageNames()
{
    if (DatabaseHandle databaseHandle = LockDatabaseAccess())
    {
        SQLite& database = databaseHandle->DB();
        db::Transaction transaction(database);

        mStageNames = database.GetRowsTuple<StageName>(fmt::format("SELECT Name, Id FROM Stages;"));
    }
}


int yaget::items::Director::AddStage(const std::string& stageName)
{
    auto stageId = Director::InvalidStageId;
    if (stageId = GetStageId(stageName); stageId == Director::InvalidStageId)
    {
        const auto command = fmt::format("INSERT INTO Stages (Name) VALUES ('{}');", stageName);

        if (DatabaseHandle databaseHandle = LockDatabaseAccess())
        {
            SQLite& database = databaseHandle->DB();
            db::Transaction transaction(database);

            if (!database.ExecuteStatement(command, nullptr))
            {
                transaction.Rollback();
                error_handlers::Throw("DIRE", fmt::format("Did not add Stage '{}' into db. %s.", stageName, ParseErrors(database)));
            }

            stageId = GetCell<int>(database, fmt::format("SELECT Id FROM Stages WHERE Name = '{}';", stageName));
            mStageNames.push_back({stageName, stageId});
        }
    }

    return stageId;
}


int yaget::items::Director::GetStageId(const std::string& stageName) const
{
    const auto it = std::ranges::find_if(mStageNames, [&stageName](auto elem)
    {
        return std::get<0>(elem) == stageName;
    });

    if (it != mStageNames.end())
    {
        const auto stageId = std::get<1>(*it);
        return stageId;
    }

    return 0;
}


