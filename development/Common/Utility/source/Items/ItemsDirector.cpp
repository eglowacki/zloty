#include "Items/ItemsDirector.h"
#include "App/AppUtilities.h"
#include <filesystem>

namespace fs = std::filesystem;

#define YAGET_ITEMS_VERSION 1


namespace
{
    std::vector<std::string> itemsSchema =
    {
        #include "Items/ItemsSchema.sqlite"
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
                std::string message = fmt::format("DIRE", "Delete database file '{}' from disk failed with error: '{}: {}'.", fileName, ec.value(), ec.message());
                YAGET_UTIL_THROW("DIRE", message.c_str());
            }
        }

        return fileName;
    }


} // namespace


//yaget::items::Director::Director(const std::string& name)
//    : mDatabase(ResolveDatabaseName(name, false), itemsSchema, YAGET_ITEMS_VERSION)
//{
//    YLOG_INFO("DIRE", "Items Director opened.");
//}


yaget::items::Director::Director(const std::string& name)
    : mDatabase(ResolveDatabaseName(name, false), itemsSchema, YAGET_ITEMS_VERSION)
{
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

        using Batch = SQLite::Row<uint64_t /*marker*/, uint64_t /*batchSize*/, uint64_t /*nextId*/>;
        Batch nextBatch = database.GetRow<Batch>("SELECT Marker, BatchSize, NextId FROM IdCache;");
        YAGET_ASSERT(nextBatch.bValid, "Did not get new id batch from items database.");

        std::string command = fmt::format("REPLACE INTO 'IdCache' (Marker, BatchSize, NextId) VALUES({}, {}, {});", nextBatch.Result, nextBatch.Result1, nextBatch.Result2 + nextBatch.Result1);
        database.ExecuteStatement(command, nullptr);

        return items::IdBatch{ nextBatch.Result2, nextBatch.Result1 };
    }

    return{ 0, 0 };
}
