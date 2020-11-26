#include "Items/ItemsDirector.h"
#include "App/AppUtilities.h"
#include "Meta/CompilerAlgo.h"
#include "Exception/Exception.h"

#include <filesystem>

namespace fs = std::filesystem;

#define YAGET_ITEMS_VERSION 4


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
        "CREATE TABLE 'VersionTables' ('Id' INTEGER NOT NULL DEFAULT 1, PRIMARY KEY('Id')) WITHOUT ROWID; "
    };

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
                YAGET_UTIL_THROW("DIRE", message);
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


yaget::items::Director::Director(const std::string& name, const Strings& additionalSchema, const Strings& loadout, int64_t expectedVersion)
    : mDatabase(ResolveDatabaseName(name, false), CombineSchemas(additionalSchema, Strings{fmt::format("INSERT INTO VersionTables(Id) VALUES('{}');", expectedVersion)}, itemsSchema), YAGET_ITEMS_VERSION)
    , mIdGameCache([this]() { return GetNextBatch(); })
{
    auto version = GetCell<int64_t>(mDatabase.DB(), "SELECT Id FROM VersionTables;");
    YAGET_UTIL_THROW_ASSERT("DIRE", (expectedVersion == Database::NonVersioned || (expectedVersion != Database::NonVersioned && version == expectedVersion)), 
        fmt::format("Director Database '{}' has mismatched version. Expected: '{}', result: '{}'.", 
            yaget::util::ExpendEnv(name, nullptr).c_str(),
            expectedVersion, 
            version));

    YLOG_INFO("DIRE", "Items Director initialized '%s'.", util::ExpendEnv(name, nullptr).c_str());

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

            YAGET_UTIL_THROW_ASSERT("DIRE", itemId != comp::INVALID_ID, fmt::format("ItemId is still invalid, is '{}' token as a first line in loadout is missing?", comp::db::NewItem_Token));
            comp::db::hash_combine(loadoutVersion, command);
            sqlLoadout.emplace_back(fmt::format(command, itemId));
        }

        const char* hashesTable = "Hashes";
        const char* hashesKey = "loadout.start";

        if (DatabaseHandle databaseHandle = LockDatabaseAccess())
        {
            SQLite& database = databaseHandle->DB();
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
                        YAGET_UTIL_THROW("DIRE", fmt::format("Could not execute sql query '{}'. {}.", command, ParseErrors(database)));
                    }
                }

                std::string sqCommand = fmt::format("INSERT OR REPLACE INTO '{}' VALUES('{}', {});", hashesTable, hashesKey, loadoutVersion);
                if (!database.ExecuteStatement(sqCommand, nullptr))
                {
                    transaction.Rollback();
                    YAGET_UTIL_THROW("DIRE", fmt::format("Could not update {} '{}' sql query '{}'. {}.", hashesTable, sqCommand, ParseErrors(database)));
                }

                YLOG_INFO("DIRE", "Items Director's first loadout is done, added: '%d' items.", sqlLoadout.size());
            }
            else if (version == loadoutVersion)
            {
                YLOG_INFO("DIRE", "Items Director's loadout is same as incomming, ignoring.");
            }
            else
            {
                YAGET_UTIL_THROW("DIRE", fmt::format("Incomming loadout version: '{}' does not match one in db: '{}'.", loadoutVersion, version));
            }
        }
    }
}

yaget::items::Director::~Director()
{
    YLOG_INFO("DIRE", "Items Director closed.");
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
        auto nextBatch = database.GetRowTuple<Batch>(nextBatchCommand, &result);
        if (!result)
        {
            transaction.Rollback();
            YAGET_UTIL_THROW("DIRE", fmt::format("Did not get next Batch from db. %s.", ParseErrors(database)));
        }

        if (!database.ExecuteStatement(updateBatchCommand, nullptr))
        {
            transaction.Rollback();
            YAGET_UTIL_THROW("DIRE", fmt::format("Did not update next Batch into db. %s.", ParseErrors(database)));
        }

        return items::IdBatch{ std::get<0>(nextBatch), std::get<1>(nextBatch) };
    }

    YAGET_UTIL_THROW("DIRE", "Did not get locked db handle for Director's database");
    return{ 0, 0 };
}
