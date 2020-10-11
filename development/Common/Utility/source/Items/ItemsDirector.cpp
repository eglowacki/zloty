#include "Items/ItemsDirector.h"
#include "App/AppUtilities.h"
#include "Meta/CompilerAlgo.h"
#include "Exception/Exception.h"

#include <filesystem>

namespace fs = std::filesystem;

#define YAGET_ITEMS_VERSION 1


namespace
{
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


yaget::items::Director::Director(const std::string& name, const Strings& additionalSchema, int64_t expectedVersion)
    : mDatabase(ResolveDatabaseName(name, false), CombineSchemas(additionalSchema, Strings{fmt::format("INSERT INTO VersionTables(Id) VALUES('{}');", expectedVersion)}, itemsSchema), YAGET_ITEMS_VERSION)
{
    auto version = GetCell<int64_t>(mDatabase.DB(), "SELECT Id FROM VersionTables;");
    YAGET_UTIL_THROW_ASSERT("DIRE", (expectedVersion == Database::NonVersioned || (expectedVersion != Database::NonVersioned && version == expectedVersion)), 
        fmt::format("Director Database '{}' has mismatched version. Expected: '{}', result: '{}'.", 
            yaget::util::ExpendEnv(name, nullptr).c_str(),
            expectedVersion, 
            version));

    YLOG_INFO("DIRE", "Items Director opened.");
}

yaget::items::Director::~Director()
{
    YLOG_INFO("DIRE", "Items Director closed.");
}


yaget::items::IdBatch yaget::items::Director::GetNextBatch()
{
    if (DatabaseHandle databaseHandle = LockDatabaseAccess())
    {
        SQLite& database = databaseHandle->DB();
        yaget::db::Transaction transaction(database);

        bool result = true;
        using Row = std::tuple<uint64_t /*marker*/, uint64_t /*batchSize*/, uint64_t /*nextId*/>;
        auto nextBatch = database.GetRowTuple<Row>("SELECT Marker, BatchSize, NextId FROM IdCache;", &result);
        YAGET_UTIL_THROW_ASSERT("DIRE", result, fmt::format("Did not get next Batch from db. %s.", ParseErrors(database)));

        const std::string command = fmt::format("REPLACE INTO 'IdCache' (Marker, BatchSize, NextId) VALUES({}, {}, {});", std::get<0>(nextBatch) , std::get<1>(nextBatch), std::get<2>(nextBatch) + std::get<1>(nextBatch));//nextBatch.Result2 + nextBatch.Result1);
        database.ExecuteStatement(command, nullptr);

        return items::IdBatch{ std::get<2>(nextBatch), std::get<1>(nextBatch) };
    }

    return{ 0, 0 };
}
