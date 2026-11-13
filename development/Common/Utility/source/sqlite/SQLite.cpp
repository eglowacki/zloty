#include "sqlite/SQLite.h"
#include "sqlite/sqlite3.h"
#include "App/AppUtilities.h"
#include "App/FileUtilities.h"
#include "Logger/YLog.h"
#include "Streams/Guid.h"

namespace
{
    //--------------------------------------------------------------------------------------------------
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
            mErrorMessage = std::format("Could not delete SQLite Database: '{}'.", fileName);
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
            YLOG_INFO("DB", "SQLite Database '%s' created. Version: '%s'", fileName, SQLITE_VERSION);
        }
        else
        {
            result = SQLITE_ERROR;
            if (mErrorMessage.empty())
            {
                mErrorMessage = std::format("Could not initialize Database Schema: '{}'. {}", fileName, sqlite3_errmsg(mDatabase));
            }
            Close();
        }
    }
    else
    {
        mErrorMessage = std::format("Could not open/create SQLite Database: '{}'. Error: '{}'.", fileName, sqlite3_errmsg(mDatabase));
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
                mErrorMessage = std::format("Last command: '{}'. Error: '{}'", command.c_str(), errorTest ? errorTest : "");
                sqlite3_free(errorTest);
                result = false;
            }
        }
        else
        {
            mErrorMessage = std::format("Command '%s' with len: '{}' exceeded size limit. Maximum length of command is '{}'.", command.c_str(), command.size(), SQLite::MAX_COMMAND_LEN);
            result = false;
        }
    }
    else
    {
        mErrorMessage = std::format("Error executing command '{}'. There is no Database created.", command.c_str());
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

template<typename Fake>
void yaget::SQLite::StatementBinder<std::string, Fake>::Bind(sqlite3* database, sqlite3_stmt* statement, const std::string& value, int index)
{
    int result = sqlite3_bind_text(statement, index, value.c_str(), -1, SQLITE_TRANSIENT);
    const char* errorMessage = sqlite3_errmsg(database);
    YAGET_ASSERT(result == SQLITE_OK, "Bind string statement failed: %s.", errorMessage ? errorMessage : "");
}

yaget::SQLite::Statement::Statement(sqlite3* database, const std::string& command) : mDatabase(database)
{
    int result = sqlite3_prepare_v2(mDatabase, command.c_str(), static_cast<int>(command.size()), &mStm, 0);
    const char* errorMessage = sqlite3_errmsg(mDatabase);
    YAGET_ASSERT(result == SQLITE_OK, "SQLite Statement '%s' prepare failed: %s.", command.c_str(), errorMessage ? errorMessage : "");
}

void yaget::SQLite::Statement::ResetBindings()
{
    sqlite3_reset(mStm);
    sqlite3_clear_bindings(mStm);
}

bool yaget::SQLite::ExecuteStatement(Statement* statement)
{
    mErrorMessage = "";
    int result = sqlite3_step(statement->mStm);
    if (result == SQLITE_OK || result == SQLITE_DONE)
    {
        return true;
    }

    mErrorMessage = std::format("Execute prepared statement error: {} - {}. Statement: {}.", result, sqlite3_errmsg(mDatabase), sqlite3_expanded_sql(statement->mStm));
    return false;
}

std::vector<std::string> yaget::SQLite::GetErrors() const
{
    std::vector<std::string> errors;
    if (!mErrorMessage.empty())
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


//------------------------------------------------------------------------------------------------------------------------------------
void yaget::SQLite::Log(const std::string& messageType, const std::string& message)
{
    using LogRecord = std::tuple<std::string /*Type*/, std::string /*Message*/, std::string /*SessionId*/>;

    LogRecord logRecord(messageType, message, util::ApplicationRuntimeId().str());
    ExecuteStatementTuple("MessageAdd", "Logs", logRecord,  { "Type", "Message", "SessionId" }, SQLite::Behaviour::Insert, SQLite::TimeStamp::Yes);
}


//--------------------------------------------------------------------------------------------------
bool yaget::SQLite::RegisterUserFunction(const std::string& functionName, UserFunctionWrapper callback, size_t numParameters)
{
    mErrorMessage = "";
    mUserFunctions.insert({functionName, callback});
    auto* wrapperCallback = &mUserFunctions[functionName];

    int result = sqlite3_create_function(mDatabase, functionName.c_str(), static_cast<int>(numParameters), SQLITE_UTF8 | SQLITE_DETERMINISTIC, wrapperCallback, &SQLUserFunctionCallback, nullptr, nullptr);
    if (result != SQLITE_OK)
    {
        mErrorMessage = std::format("Create user function error: '{}'. Function: '{}', error: '{}'.", result, functionName, sqlite3_errmsg(mDatabase));
        return false;
    }

    return true;
}


//--------------------------------------------------------------------------------------------------
void yaget::SQLite::SQLUserFunctionCallback(sqlite3_context* context, int argc, sqlite3_value** argv)
{
    auto userFunctionWrapper = static_cast<UserFunctionWrapper*>(sqlite3_user_data(context));

    if (!userFunctionWrapper)
    {
        YAGET_ASSERT(userFunctionWrapper, "Invalid user function wrapper");
        sqlite3_result_error(context, "Invalid user function wrapper", -1);
        return;
    }

    yaget::Strings parameters;
    for (int i = 0; i < argc; ++i)
    {
        std::string table = reinterpret_cast<const char*>(sqlite3_value_text(argv[i]));
        parameters.push_back(table);
    }

    auto result = (*userFunctionWrapper)(parameters);
    sqlite3_result_text(context, result.c_str(), static_cast<int>(result.size()), SQLITE_TRANSIENT);
}



////------------------------------------------------------------------------------------------------------------------------------------
//bool yaget::SQLite::AddUserFunction(const std::string& functionName, UserFunction userFunction)
//{
//    userFunction;
//
//    int result = sqlite3_create_function(mDatabase, functionName.c_str(), 4, SQLITE_UTF8 | SQLITE_DETERMINISTIC, this, &SqUserFunction, nullptr, nullptr);
//    if (result != SQLITE_OK)
//    {
//        mErrorMessage = std::format("Create user function error: '{}'. Function: '{}', error: '{}'.", result, functionName, sqlite3_errmsg(mDatabase));
//        return false;
//        
//    }
//
//    return true;
//
//
//  //sqlite3 *db,
//  //const char *zFunctionName,
//  //int nArg,
//  //int eTextRep,
//  //void *pApp,
//  //void (*xFunc)(sqlite3_context*,int,sqlite3_value**),
//  //void (*xStep)(sqlite3_context*,int,sqlite3_value**),
//  //void (*xFinal)(sqlite3_context*)
//
////int rc = sqlite3_create_function(db, "increment", 1, SQLITE_UTF8, NULL, &increment, NULL, NULL);
////static void changeNotify(sqlite_func* context, int argc, const char** argv)
////const char *table, *action, *rowid;
////
////assert(argc == 3);
////
////table = argv[0];
////action = argv[1];
////rowid = argv[2];
////
////dprintf("Database changed: table = %s, action = %s, rowid = %s\n", table, action, rowid);
//
//}


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
