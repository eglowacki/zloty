#include "Database/Database.h"
#include "App/AppUtilities.h"
#include "App/FileUtilities.h"
#include "fmt/format.h"
#include "Logger/YLog.h"
#include "sqlite/SQLite.h"
#include "sqlite/sqlite3.h"

#include <filesystem>

namespace
{
    //-----------------------------------------------------------------------------------------------------------------------------------------------
    bool InitializeSchema(yaget::SQLite& database, sqlite3* /*sqlite*/, const std::vector<std::string>& schema)
    {
        using namespace yaget;

        for (const auto& it : schema)
        {
            if (!database.ExecuteStatement(it, nullptr))
            {
                YLOG_ERROR("DB", "Could not execute sql query '%s'. %s.", it.c_str(), conv::Combine(database.GetErrors(), "\n").c_str());
                return false;
            }
        }

        return true;
    }

    bool InitializeEmptySchema(yaget::SQLite& /*database*/, sqlite3* /*sqlite*/, const std::vector<std::string>& /*schema*/)
    {
        return true;
    }

    void InitializeDatabase(yaget::SQLite& database, const std::vector<std::string>& schema, const std::string& databaseName, size_t excpectedVersion)
    {
        using namespace yaget;
        const std::string fileName = yaget::util::ExpendEnv(databaseName, nullptr);
        std::string dbFileName = fileName.empty() ? ":memory:" : fileName;
        SQLite::DatabaseType dbAsType = SQLite::DatabaseType::InMemory;

        if (!fileName.empty())
        {
            bool bFileExist = io::file::IsFileExists(fileName);
            if (bFileExist)
            {
                dbAsType = SQLite::DatabaseType::Append;
            }
            else
            {
                dbAsType = SQLite::DatabaseType::New;
            }

            if (!bFileExist)
            {
                io::file::AssureDirectories(fileName);
            }
        }

        using namespace std::placeholders;
        auto callback = dbAsType == SQLite::DatabaseType::Append ? std::bind(InitializeEmptySchema, _1, _2, schema) : std::bind(InitializeSchema, _1, _2, schema);
        if (database.Open(dbFileName.c_str(), dbAsType, callback))
        {
            if (excpectedVersion != Database::NonVersioned)
            {
                auto dbVersion = GetCell<size_t>(database, "SELECT Id FROM Version;");
                if (dbVersion != excpectedVersion)
                {
                    const auto& textError = fmt::format("Database '{}' has mismatched version. Expected: '{}', result: '{}'.", fileName, excpectedVersion, dbVersion);
                    YAGET_UTIL_THROW("DB", textError);
                }
            }
        }
        else
        {
            const auto& textError = fmt::format("Did not initialize database '{}'. {}.", fileName, conv::Combine(database.GetErrors(), "\n"));
            YAGET_UTIL_THROW("DB", textError);
        }
    }                                                                                           

} // namespace

yaget::Database::Database(const std::string& name, const std::vector<std::string>& schema, size_t excpectedVersion)
{
    InitializeDatabase(mDatabase, schema, name, excpectedVersion);
}

yaget::Database::~Database()
{
}

void yaget::Database::Log(const std::string& type, const std::string& message)
{
    mDatabase.Log(type, message);
}
