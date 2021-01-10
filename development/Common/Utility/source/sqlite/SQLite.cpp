#include "sqlite/SQLite.h"
#include "sqlite/sqlite3.h"
#include "App/AppUtilities.h"
#include "App/FileUtilities.h"
#include "Fmt/format.h"
#include "Logger/YLog.h"
#include "Streams/Guid.h"

namespace
{
    int SqlQueryCallback(void *userData, int argc, char **argv, char **columnNames)
    {
        if (userData)
        {
            yaget::SQLite::QueryCallback *callback = static_cast<yaget::SQLite::QueryCallback*>(userData);

            return (*callback)(argc, argv, columnNames);
        }
        else
        {
            return -1;
        }
    }

} // namespace

yaget::SQLite::SQLite()
    : mDatabase(nullptr)
{
}

yaget::SQLite::~SQLite()
{
    Close();
}

bool yaget::SQLite::Open(const char* fileName, DatabaseType openDatabaseAsType, InitializeSchema_t initializeSchemaCallback)
{
    const bool serializedModel = false;

    mErrorMessage = "";
    Close();

    int dbFlags = serializedModel ? SQLITE_OPEN_FULLMUTEX : 0;
    if (openDatabaseAsType == DatabaseType::New)
    {
        dbFlags |= SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE;

        if (io::file::IsFileExists(fileName) && std::remove(fileName) != 0)
        {
            mErrorMessage = fmt::format("Could not delete SQLite Database: '{}'.", fileName);
            return false;
        }
    }
    else if (openDatabaseAsType == DatabaseType::Append || openDatabaseAsType == DatabaseType::InMemory)
    {
        dbFlags |= SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE;
    }
    else
    {
        dbFlags |= SQLITE_OPEN_READONLY;
    }

    int result = sqlite3_open_v2(fileName, &mDatabase, dbFlags, NULL);
    if (result == SQLITE_OK)
    {
        ExecuteStatement("PRAGMA foreign_keys = ON;", nullptr);

        Batcher batcher(*this);
        if (initializeSchemaCallback && initializeSchemaCallback(*this, mDatabase))
        {
            YLOG_INFO("SQL", "SQLite Database '%s' created. Version: '%s'", fileName, SQLITE_VERSION);
        }
        else
        {
            result = SQLITE_ERROR;
            if (mErrorMessage.empty())
            {
                mErrorMessage = fmt::format("Could not initialize Database Schema: '{}'. {}", fileName, sqlite3_errmsg(mDatabase));
            }
            Close();
        }
    }
    else
    {
        mErrorMessage = fmt::format("Could not open/create SQLite Database: '{}'. Error: '{}'.", fileName, sqlite3_errmsg(mDatabase));
        Close();
    }

    return result == SQLITE_OK;
}

void yaget::SQLite::Close()
{
    for (auto&& it : mStatements)
    {
        Statement& statement = it.second;
        sqlite3_finalize(statement.mStm);
        statement.mStm = nullptr;
    }

    mStatements.clear();
    sqlite3_close(mDatabase);
    mDatabase = nullptr;
}

bool yaget::SQLite::ExecuteStatement(const std::string& command, QueryCallback *callback)
{
    YAGET_ASSERT(mDatabase, ("SQLite::ExecuteStatement: '%s' called, but sqlite db is not created yet.", command.c_str()));

    mErrorMessage = "";
    bool result = false;

    if (mDatabase)
    {
        if (command.size() < SQLite::MAX_COMMAND_LEN)
        {
            char* errorTest = nullptr;
            int execResult = sqlite3_exec(mDatabase, command.c_str(), (callback ? SqlQueryCallback : nullptr), callback, &errorTest);
            if (execResult == SQLITE_OK || execResult == SQLITE_ABORT)
            {
                sqlite3_free(errorTest);
                result = true;
            }
            else
            {
                mErrorMessage = fmt::format("Last command: {}. Error: {}", command.c_str(), errorTest ? errorTest : "");
                sqlite3_free(errorTest);
                result = false;
            }
        }
        else
        {
            mErrorMessage = fmt::format("Command '%s' with len: '{}' exceeded size limit. Maximum length of command is '{}'.", command.c_str(), command.size(), SQLite::MAX_COMMAND_LEN);
            result = false;
        }
    }
    else
    {
        mErrorMessage = fmt::format("Error executing command {}. There is no Database created.", command.c_str());
        result = false;
    }

    return result;
}

void yaget::SQLite::StatementBinder<yaget::null_marker_t>::Bind(sqlite3* database, sqlite3_stmt* statement, yaget::null_marker_t /*value*/, int index)
{
    int result = sqlite3_bind_null(statement, index);
    const char* errorMessage = sqlite3_errmsg(database);
    YAGET_ASSERT(result == SQLITE_OK, "Bind null statement failed: %s.", errorMessage ? errorMessage : "");
}

void yaget::SQLite::StatementBinder<int>::Bind(sqlite3* database, sqlite3_stmt* statement, int value, int index)
{
    int result = sqlite3_bind_int(statement, index, value);
    const char* errorMessage = sqlite3_errmsg(database);
    YAGET_ASSERT(result == SQLITE_OK, "Bind int statement failed: %s.", errorMessage ? errorMessage : "");
}

void yaget::SQLite::StatementBinder<int64_t>::Bind(sqlite3* database, sqlite3_stmt* statement, int64_t value, int index)
{
    int result = sqlite3_bind_int64(statement, index, value);
    const char* errorMessage = sqlite3_errmsg(database);;
    YAGET_ASSERT(result == SQLITE_OK, "Bind int statement failed: %s.", errorMessage ? errorMessage : "");
}

void yaget::SQLite::StatementBinder<uint64_t>::Bind(sqlite3* database, sqlite3_stmt* statement, uint64_t value, int index)
{
    StatementBinder<int64_t>::Bind(database, statement, static_cast<int64_t>(value), index);
    //int result = sqlite3_bind_int64(statement, index, value);
    //const char* errorMessage = sqlite3_errmsg(database);;
    //YAGET_ASSERT(result == SQLITE_OK, "Bind int statement failed: %s.", errorMessage ? errorMessage : "");
}

void yaget::SQLite::StatementBinder<bool>::Bind(sqlite3* database, sqlite3_stmt* statement, bool value, int index)
{
    int result = sqlite3_bind_int(statement, index, value);
    const char* errorMessage = sqlite3_errmsg(database);
    YAGET_ASSERT(result == SQLITE_OK, "Bind bool (int) statement failed: %s.", errorMessage ? errorMessage : "");
}

void yaget::SQLite::StatementBinder<float>::Bind(sqlite3* database, sqlite3_stmt* statement, float value, int index)
{
    int result = sqlite3_bind_double(statement, index, value);
    const char* errorMessage = sqlite3_errmsg(database); 
    YAGET_ASSERT(result == SQLITE_OK, "Bind float statement failed: %s.", errorMessage ? errorMessage : "");
}

void yaget::SQLite::StatementBinder<yaget::Guid>::Bind(sqlite3* database, sqlite3_stmt* statement, yaget::Guid value, int index)
{
    StatementBinder<std::string>::Bind(database, statement, value.str(), index);
}

void yaget::SQLite::StatementBinder<std::vector<std::string>>::Bind(sqlite3* database, sqlite3_stmt* statement, const std::vector<std::string>& value, int index)
{
    StatementBinder<std::string>::Bind(database, statement, conv::Convertor<std::vector<std::string>>::ToString(value), index);
}

void yaget::SQLite::StatementBinder<math3d::Vector3>::Bind(sqlite3* database, sqlite3_stmt* statement, const math3d::Vector3& value, int index)
{
    StatementBinder<std::string>::Bind(database, statement, conv::Convertor<math3d::Vector3>::ToString(value), index);
}

void yaget::SQLite::StatementBinder<math3d::Quaternion>::Bind(sqlite3* database, sqlite3_stmt* statement, const math3d::Quaternion& value, int index)
{
    StatementBinder<std::string>::Bind(database, statement, conv::Convertor<math3d::Quaternion>::ToString(value), index);
}

void yaget::SQLite::StatementBinder<std::string>::Bind(sqlite3* database, sqlite3_stmt* statement, const std::string& value, int index)
{
    int result = sqlite3_bind_text(statement, index, value.c_str(), -1, SQLITE_TRANSIENT);
    const char* errorMessage = sqlite3_errmsg(database);
    YAGET_ASSERT(result == SQLITE_OK, "Bind string statement failed: %s.", errorMessage ? errorMessage : "");
}

yaget::SQLite::Statement::Statement(sqlite3* database, const std::string& command) : mDatabase(database)
{
    int result = sqlite3_prepare_v2(mDatabase, command.c_str(), static_cast<int>(command.size()), &mStm, 0);
    const char* errorMessage = sqlite3_errmsg(mDatabase);
    YAGET_ASSERT(result == SQLITE_OK, "SQLite Statement prepare failed: %s.", errorMessage ? errorMessage : "");
}

void yaget::SQLite::Statement::ResetBindings()
{
    sqlite3_reset(mStm);
    sqlite3_clear_bindings(mStm);
}

bool yaget::SQLite::ExecuteStatement(Statement* statement)
{
    int result = sqlite3_step(statement->mStm);
    if (result == SQLITE_OK || result == SQLITE_DONE)
    {
        return true;
    }

    mErrorMessage = fmt::format("Execute prepared statement error: {}", sqlite3_errmsg(mDatabase));
    return false;
}

std::vector<std::string> yaget::SQLite::GetErrors() const
{
    std::vector<std::string> errors;
    if (mErrorMessage.size())
    {
        errors.push_back(mErrorMessage);
    }

    return errors;
}

bool yaget::SQLite::Backup(const char *fileName) const
{
    sqlite3* pFile = nullptr;;
    int rc = sqlite3_open(fileName, &pFile);
    if (rc == SQLITE_OK)
    {
        if (sqlite3_backup* pBackup = sqlite3_backup_init(pFile, "main", mDatabase, "main"))
        {
            sqlite3_backup_step(pBackup, -1);
            sqlite3_backup_finish(pBackup);

            return true;
        }
    }

    return false;
}

void yaget::SQLite::Log(const std::string& messageType, const std::string& message)
{
    using LogRecord = std::tuple<std::string /*Type*/, std::string /*Message*/, std::string /*SessionId*/>;

    LogRecord logRecord(messageType, message, util::ApplicationRuntimeId().str());
    ExecuteStatementTuple("MessageAdd", "Logs", logRecord,  { "Type", "Message", "SessionId" }, SQLite::Behaviour::Insert, SQLite::TimeStamp::Yes);
}


//------------------------------------------------------------------------------------------------------------------------------------
yaget::db::Transaction::Transaction(SQLite& database) : mDatabase(database)
{
    const auto result = mDatabase.ExecuteStatement("BEGIN", nullptr);
    YAGET_ASSERT(result, "BEGIN TRANSCATION failed. %s", ParseErrors(mDatabase).c_str());
}

void yaget::db::Transaction::Rollback()
{
    mCommit = false;
}

yaget::db::Transaction::~Transaction()
{
    if (mCommit)
    {
        const auto result = mDatabase.ExecuteStatement("END", nullptr);
        YAGET_ASSERT(result, "END TRANSCATION failed. %s", ParseErrors(mDatabase).c_str());
    }
    else
    {
        const auto result = mDatabase.ExecuteStatement("ROLLBACK", nullptr);
        YAGET_ASSERT(result, "ROLLBACK TRANSCATION failed. %s", ParseErrors(mDatabase).c_str());
    }
}

