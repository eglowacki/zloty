//////////////////////////////////////////////////////////////////////
// SQLite.h
//
//  Copyright 6/16/2019 Edgar Glowacki
//
//  Maintained by: Edgar
//
//  NOTES:
//      Lost in time for this header 
//
//
//  #include "sqlite/SQLite.h"
//
//////////////////////////////////////////////////////////////////////
//! \file
#pragma once

#include "StringHelpers.h"
#include "sqlite/AccessHelpers.h"

#include <functional>
#include <map>
#include <vector>

struct sqlite3;
struct sqlite3_stmt;

namespace yaget
{
    class SQLite;

    // Used in SQLite::Row to bind NULL to bindable parameter in prepared statement
    typedef struct null_marker
    {
        bool operator==(const null_marker& /*other*/) const
        {
            return true;
        }

    } null_marker_t;

    //-----------------------------------------------------------------------------------------------------------------------------------------------
    std::string ParseErrors(const SQLite& database);

    //-----------------------------------------------------------------------------------------------------------------------------------------------
    /**
     Thin wrapper around sqlite library, to exposes some simple query and management of db
    */
    class SQLite
    {
    public:
        enum class DatabaseType { New, Append, ReadOnly, InMemory };

        // This is maximum size for db execute command.
        static const uint32_t MAX_COMMAND_LEN = 1000000;// 4096;

        //-------------------------------------------------------------------------------------------------------------------------------------------
        struct QueryCallback
        {
            virtual ~QueryCallback() = default;
            // Return 0 to continue getting more rows, any other value, stop querying.
            virtual int operator()(int numValues, char **values, char **columnNames) = 0;
        };

        struct Batcher
        {
            Batcher(SQLite& database) : mDatabase(database)
            {
                const auto result = mDatabase.ExecuteStatement("BEGIN TRANSACTION", nullptr);
                YAGET_ASSERT(result, "Batcher %s", yaget::ParseErrors(mDatabase).c_str());
            }

            ~Batcher()
            {
                // if DB got closed, we do not want to generate any new errors for this call
                if (mDatabase.mDatabase)
                {
                    const auto result = mDatabase.ExecuteStatement("END TRANSACTION", nullptr);
                    YAGET_ASSERT(result, "Batcher %s", yaget::ParseErrors(mDatabase).c_str());
                }
            }

        private:
            SQLite& mDatabase;
        };

        //-------------------------------------------------------------------------------------------------------------------------------------------
        SQLite();
        ~SQLite();

        typedef std::function<bool(SQLite& database, sqlite3 *sqlite)> InitializeSchema_t;

        bool Open(const char *fileName, DatabaseType openDatabaseAsType, InitializeSchema_t initializeSchemaCallback);
        void Close();

        bool ExecuteStatement(const std::string& command, QueryCallback *callback);

        // Update if record already exist, otherwise insert, inset will not update existing record
        enum class Behaviour { Update, Insert };
        enum class TimeStamp { Yes, No };
        typedef std::string StatementId_t;

        template<typename T>
        void PreCacheStatementTuple(const StatementId_t& statementId, const std::string& tableName, const Strings& columnNames, Behaviour behaviour, TimeStamp automaticTime = TimeStamp::No);
        template<typename T>
        bool ExecuteStatementTuple(const StatementId_t& statementId, const std::string& tableName, const T& dataRow, const Strings& columnNames, Behaviour behaviour, TimeStamp automaticTime = TimeStamp::No);
        template<typename T>
        bool ExecuteStatementTuple(const StatementId_t& statementId, const T& dataRow);

        bool IsStatementCached(const StatementId_t& statementId) const { return mStatements.find(statementId) != std::end(mStatements); }

        template<typename T>
        T GetRowTuple(const std::string& command, bool* result) const noexcept(false);
        template<typename T>
        std::vector<T> GetRowsTuple(const std::string& command) const;

        template <typename R, typename Q, typename C = std::vector<R>>
        C GetRowsTuple(const std::string& command, std::function<R(Q)> converter) const;

        // Utility methods
        bool Backup(const char *fileName) const;

        std::vector<std::string> GetErrors() const;

        void Log(const std::string& messageType, const std::string& message);

    private:
        // this templates execute binding call to sqlite per specific type
        template<typename T> struct StatementBinder;
        template<> struct StatementBinder<conv::unused_marker_t> {static void Bind(sqlite3* /*database*/, sqlite3_stmt* /*statement*/, conv::unused_marker_t /*value*/, int /*index*/) {}};
        template<> struct StatementBinder<null_marker_t> {static void Bind(sqlite3* database, sqlite3_stmt* statement, null_marker_t value, int index);};
        template<> struct StatementBinder<bool> { static void Bind(sqlite3* database, sqlite3_stmt* statement, bool value, int index); };
        template<> struct StatementBinder<int> {static void Bind(sqlite3* database, sqlite3_stmt* statement, int value, int index);};
        template<> struct StatementBinder<int64_t> {static void Bind(sqlite3* database, sqlite3_stmt* statement, int64_t value, int index);};
        template<> struct StatementBinder<uint64_t> {static void Bind(sqlite3* database, sqlite3_stmt* statement, uint64_t value, int index);};
        template<> struct StatementBinder<float> {static void Bind(sqlite3* database, sqlite3_stmt* statement, float value, int index);};
        template<> struct StatementBinder<yaget::Guid> { static void Bind(sqlite3* database, sqlite3_stmt* statement, yaget::Guid value, int index); };
        template<> struct StatementBinder<std::string> {static void Bind(sqlite3* database, sqlite3_stmt* statement, const std::string& value, int index);};
        template<> struct StatementBinder<std::vector<std::string>> { static void Bind(sqlite3* database, sqlite3_stmt* statement, const std::vector<std::string>& value, int index); };
        template<> struct StatementBinder<math3d::Vector3> { static void Bind(sqlite3* database, sqlite3_stmt* statement, const math3d::Vector3& value, int index); };
        template<> struct StatementBinder<math3d::Quaternion> { static void Bind(sqlite3* database, sqlite3_stmt* statement, const math3d::Quaternion& value, int index); };

        // this will bind specific value type to sql statement
        template <
            size_t Index = 0,
            typename TTuple,
            size_t Size = std::tuple_size_v<std::remove_reference_t<TTuple>>
        >
        constexpr static void for_each(TTuple&& tuple, sqlite3* database, sqlite3_stmt* stm)
        {
            if constexpr (Index < Size)
            {
                using TT = typename std::remove_pointer<typename std::decay<decltype(std::get<Index>(tuple))>::type>::type;
                StatementBinder<TT>::Bind(database, stm, std::get<Index>(tuple), Index + 1);    // last param, index is always based on 1

                if constexpr (Index + 1 < Size)
                {
                    for_each<Index + 1>(std::forward<TTuple>(tuple), database, stm);
                }
            }
        }

        // Manage bindable parameters
        struct Statement
        {
            Statement() = default;
            Statement(sqlite3* database, const std::string& command);

            template<typename T>
            void UpdateStatementTuple(const T& dataValues)
            {
                ResetBindings();
                for_each(dataValues, mDatabase, mStm);
            }

            void ResetBindings();

            sqlite3_stmt* mStm = nullptr;
            sqlite3* mDatabase = nullptr;
        };

        bool ExecuteStatement(Statement* statement);

        sqlite3* mDatabase;
        mutable std::string mErrorMessage;

        // Mapping between string (provided by user) and prepared statement. Once Statement is constructed, it can be executed faster for subsequent
        // runs and provides better input validation
        // NOTE EG: use something else for id rather then string
        using Statements = std::map<StatementId_t, Statement>;
        Statements mStatements;
    };

    inline std::string ParseErrors(const SQLite& database)
    {
        const auto& errorList = database.GetErrors();
        const auto& result = conv::Combine(errorList, ",");

        return result.empty() ? "UNKNOWN ERROR" : result;
    }

    // Helper to return one value from query
    template<typename T>
    T GetCell(const SQLite& database, const std::string& command, const T& defaultValue = T())
    {
        using Row = std::tuple<T>;
        Row result = Row(defaultValue);

        result = database.GetRowTuple<Row>(command, nullptr);

        return std::get<0>(result);
    }


    //-----------------------------------------------------------------------------------------------------------------------------------------------
    namespace internal
    {
        enum class QueryGetter { Single, Multi };

        template<typename T, QueryGetter Q>
        struct RowGetter : SQLite::QueryCallback
        {
            using Row = T;

            int operator()(int numValues, char **values, char** /*columnNames*/) override
            {
                YAGET_ASSERT(numValues == std::tuple_size_v<std::remove_reference_t<Row>>);

                Row row{};
                size_t index = 0;
                meta::for_each(row, [values, &index]<typename T0>(T0& element)
                {
                    element = conv::Convertor<T0>::FromString(values[index]);
                    ++index;
                });

                mRowData.emplace_back(row);
                return Q == QueryGetter::Multi ? 0 : -1;
            }

            std::vector<Row> mRowData;
        };

    } // namespace internal

    //-----------------------------------------------------------------------------------------------------------------------------------------------
    template<typename T>
    bool SQLite::ExecuteStatementTuple(const StatementId_t& statementId, const std::string& tableName, const T& dataRow, const Strings& columnNames, Behaviour behaviour, TimeStamp automaticTime /*= TimeStamp::No*/)
    {
        YAGET_ASSERT(mDatabase, "SQLite::ExecuteStatement<RT>: '%s' called for table: '%s', but sqlite db is not created yet.", statementId.c_str(), tableName.c_str());

        if (const auto it = mStatements.find(statementId); it == mStatements.end())
        {
            PreCacheStatementTuple<T>(statementId, tableName, columnNames, behaviour, automaticTime);
        }

        return ExecuteStatementTuple<T>(statementId, dataRow);
    }

    //-----------------------------------------------------------------------------------------------------------------------------------------------
    template<typename T>
    void SQLite::PreCacheStatementTuple(const StatementId_t& statementId, const std::string& tableName, const Strings& columnNames, Behaviour behaviour, TimeStamp automaticTime /*= TimeStamp::No*/)
    {
        YAGET_ASSERT(mDatabase, "SQLite::ExecuteStatement<RT>: '%s' called for table: '%s', but sqlite db is not created yet.", statementId.c_str(), tableName.c_str());
        YAGET_ASSERT(mStatements.find(statementId) == std::end(mStatements), "Can not create new statement '%s' since it already exists.", conv::Convertor<std::string>::ToString(statementId).c_str());

        std::string timeStamp;
        if (automaticTime == TimeStamp::Yes)
        {
            timeStamp = ", TimeStamp";
        }

        int numValues = std::tuple_size<T>::value - 1;
        const char* updateStr = behaviour == Behaviour::Update ? "OR REPLACE" : "";
        std::string command = fmt::format("INSERT {} INTO '{}' ({}{}) VALUES (?", updateStr, tableName, conv::Combine(columnNames, ", "), timeStamp);
        while (numValues)
        {
            command += ", ?";
            --numValues;
        }

        if (automaticTime == TimeStamp::Yes)
        {
            command += ", TIME('now', 'localtime')";
        }

        command += ");";
        mStatements[statementId] = Statement(mDatabase, command);
    }

    //-----------------------------------------------------------------------------------------------------------------------------------------------
    template<typename T>
    bool SQLite::ExecuteStatementTuple(const StatementId_t& statementId, const T& dataRow)
    {
        YAGET_ASSERT(mStatements.find(statementId) != std::end(mStatements), "Can not execute statement '%s' because it's not pre-cached ", conv::Convertor<std::string>::ToString(statementId).c_str());

        auto* statement = &mStatements[statementId];
        statement->UpdateStatementTuple(dataRow);
        return ExecuteStatement(statement);
    }

    //-----------------------------------------------------------------------------------------------------------------------------------------------
    template<typename T>
    T SQLite::GetRowTuple(const std::string& command, bool* result) const noexcept(false)
    {
        internal::RowGetter<T, internal::QueryGetter::Single> row;
        const bool executed = const_cast<SQLite&>(*this).ExecuteStatement(command, &row);
        if (result)
        {
            *result = executed && !row.mRowData.empty();
        }

        return row.mRowData.empty() ? T{} : *row.mRowData.begin();
    }

    //-----------------------------------------------------------------------------------------------------------------------------------------------
    template<typename T>
    std::vector<T> SQLite::GetRowsTuple(const std::string& command) const
    {
        internal::RowGetter<T, internal::QueryGetter::Multi> rows;
        if (!const_cast<SQLite&>(*this).ExecuteStatement(command, &rows))
        {
            return {};
        }

        return rows.mRowData;
    }

    //-----------------------------------------------------------------------------------------------------------------------------------------------
    template <typename R, typename Q, typename C>
    C SQLite::GetRowsTuple(const std::string& command, std::function<R(Q)> converter) const
    {
        std::vector<Q> dbResults = GetRowsTuple<Q>(command);
        C container;

        std::transform(dbResults.begin(), dbResults.end(), std::inserter(container, container.end()), [converter](const Q& record)
        {
            return converter(record);
        });

        return container;
    }

    namespace db
    {
        //--------------------------------------------------------------------------------------------------
        // Makes all db commands atomic, all succeed or none, used only be derived classes and internal support functions
        struct Transaction
        {
            Transaction(SQLite& database);
            ~Transaction();
            void Rollback();

        private:
            SQLite& mDatabase;
            bool mCommit = true;
        };

    } // namespace db

} // namespace yaget
