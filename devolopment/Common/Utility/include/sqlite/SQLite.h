#pragma once

#include "Debugging/Assert.h"
#include "StringHelpers.h"
#include <functional>
#include <vector>
#include <map>

struct sqlite3;
struct sqlite3_stmt;

namespace yaget
{
    class SQLite;
    //// Used in SQLite::Row struct to mark up Tx fields as non-used
    //typedef struct unused_marker 
    //{
    //    bool operator==(const unused_marker& /*other*/) const
    //    {
    //        return true;
    //    }

    //} unused_marker_t;

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
    class SQLite// : public CMultiThreadRefCount
    {
    public:
        enum class DatabaseType { DT_NEW, DT_APPEND, DT_READONLY, DT_IN_MEMORY };

        // This is maximum size for db execute command.
        static const uint32_t MAX_COMMAND_LEN = 1000000;// 4096;

        //-------------------------------------------------------------------------------------------------------------------------------------------
        struct QueryCallback
        {
            virtual ~QueryCallback() {}
            // Return 0 to continue getting more rows, any other value, stop querying.
            virtual int operator()(int numValues, char **values, char **columnNames) = 0;
        };

        struct Batcher
        {
            Batcher(SQLite& database) : m_database(database)
            {
                bool result = m_database.ExecuteStatement("BEGIN TRANSACTION", nullptr);
                YAGET_ASSERT(result, "Batcher %s", yaget::ParseErrors(m_database).c_str());
            }

            ~Batcher()
            {
                // if DB got closed, we do not want to generate any new errors for this call
                if (m_database.mDatabase)
                {
                    bool result = m_database.ExecuteStatement("END TRANSACTION", nullptr);
                    YAGET_ASSERT(result, "Batcher %s", yaget::ParseErrors(m_database).c_str());
                }
            }

        private:
            SQLite& m_database;
        };

        //-------------------------------------------------------------------------------------------------------------------------------------------
        SQLite();
        ~SQLite();

        typedef std::function<bool(SQLite& database, sqlite3 *sqlite)> InitializeSchema_t;

        bool Open(const char *fileName, DatabaseType openDatabaseAsType, InitializeSchema_t initializeSchemaCallback);
        void Close();

        bool ExecuteStatement(const std::string& command, QueryCallback *callback);

        // Update if record already exist, otherwise insert, inset will not update existing record
        enum class Behaviour { Update, Insert, TimeStampYes, NoTimeStamp };
        typedef std::string StatementId_t;
        template<typename RT>
        bool ExecuteStatement(const StatementId_t& statementId, const std::string& tableName, const RT& dataValues, Behaviour automaticTime, Behaviour behaviour);

        //-------------------------------------------------------------------------------------------------------------------------------------------
        // NOTE: EG: we could possibly use variadic args...
        template<typename T, typename T1 = conv::unused_marker_t, typename T2 = conv::unused_marker_t, typename T3 = conv::unused_marker_t, typename T4 = conv::unused_marker_t, typename T5 = conv::unused_marker_t, typename T6 = conv::unused_marker_t>
        struct Row
        {
            typedef T T; typedef T1 T1; typedef T2 T2; typedef T3 T3; typedef T4 T4; typedef T5 T5; typedef T6 T6;

            Row() : Result(), Result1(), Result2(), Result3(), Result4(), Result5(), Result6(), bValid(true)
            {}

            Row(const T& t, const T1& t1 = T1(), const T2& t2 = T2(), const T3& t3 = T3(), const T4& t4 = T4(), const T5& t5 = T5(), const T6& t6 = T6()) : Result(t), Result1(t1), Result2(t2), Result3(t3), Result4(t4), Result5(t5), Result6(t6), bValid(true)
            {}

            bool operator==(const Row<T, T1, T2, T3, T4, T5, T6>& Other) const
            {
                return Result == Other.Result &&
                    Result1 == Other.Result1 &&
                    Result2 == Other.Result2 &&
                    Result3 == Other.Result3 &&
                    Result4 == Other.Result4 &&
                    Result5 == Other.Result5 &&
                    Result6 == Other.Result6;
            }

            bool operator!=(const Row<T, T1, T2, T3, T4, T5, T6>& Other) const
            {
                return !(*this == Other);
            }

            int static NumDim()
            {
                if (typeid(T6) != typeid(conv::unused_marker_t))
                {
                    return 7;
                }
                else if (typeid(T5) != typeid(conv::unused_marker_t))
                {
                    return 6;
                }
                else if (typeid(T4) != typeid(conv::unused_marker_t))
                {
                    return 5;
                }
                else if (typeid(T3) != typeid(conv::unused_marker_t))
                {
                    return 4;
                }
                else if (typeid(T2) != typeid(conv::unused_marker_t))
                {
                    return 3;
                }
                else if (typeid(T1) != typeid(conv::unused_marker_t))
                {
                    return 2;
                }

                return 1;
            }

            T Result;
            T1 Result1;
            T2 Result2;
            T3 Result3;
            T4 Result4;
            T5 Result5;
            T6 Result6;
            bool bValid;	// for now, this only used for GetRow(...) and GetCell(...) and it is ignored in GetRows(...)
        };

        template<typename RT>
        RT GetRow(const std::string& command) const;

        template<typename RT>
        std::vector<RT> GetRows(const std::string& command) const;


        template <typename R, typename Q, typename C = std::vector<R>>
        C GetRows(const std::string& command, std::function<R(Q)> converter) const;


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
        template<> struct StatementBinder<float> {static void Bind(sqlite3* database, sqlite3_stmt* statement, float value, int index);};
        template<> struct StatementBinder<yaget::Guid> { static void Bind(sqlite3* database, sqlite3_stmt* statement, yaget::Guid value, int index); };
        template<> struct StatementBinder<std::string> {static void Bind(sqlite3* database, sqlite3_stmt* statement, const std::string& value, int index);};
        template<> struct StatementBinder<std::vector<std::string>> { static void Bind(sqlite3* database, sqlite3_stmt* statement, const std::vector<std::string>& value, int index); };

        // Manage bindable parameters
        struct Statement
        {
            Statement() {}
            Statement(sqlite3* database, const std::string& command);

            template<typename RT>
            void UpdateStatement(const RT& dataValues)
            {
                ResetBindings();

                int numValues = RT::NumDim();
                switch (numValues)
                {
                case 7:
                    StatementBinder<RT::T6>::Bind(mDatabase, m_stm, dataValues.Result6, 7);
                case 6:
                    StatementBinder<RT::T5>::Bind(mDatabase, m_stm, dataValues.Result5, 6);
                case 5:
                    StatementBinder<RT::T4>::Bind(mDatabase, m_stm, dataValues.Result4, 5);
                case 4:
                    StatementBinder<RT::T3>::Bind(mDatabase, m_stm, dataValues.Result3, 4);
                case 3:
                    StatementBinder<RT::T2>::Bind(mDatabase, m_stm, dataValues.Result2, 3);
                case 2:
                    StatementBinder<RT::T1>::Bind(mDatabase, m_stm, dataValues.Result1, 2);
                case 1:
                    StatementBinder<RT::T>::Bind(mDatabase, m_stm, dataValues.Result, 1);
                }
            }

            void ResetBindings();

            sqlite3_stmt* m_stm = nullptr;
            sqlite3* mDatabase = nullptr;
        };

        bool ExecuteStatement(Statement* statement);

        sqlite3* mDatabase;
        mutable std::string m_errorMsg;

        // Mapping between string (provided by user) and prepared statement. Once Statement is constructed, it can be executed faster for subsequent
        // runs and provides better input validation
        // NOTE EG: use something else for id rather then string
        typedef std::map<StatementId_t, Statement> Statements_t;
        Statements_t m_statements;
    };

    inline std::string ParseErrors(const SQLite& database)
    {
        std::vector<std::string> errorList = database.GetErrors();
        std::string result = conv::Combine(errorList, ",");

        return result.empty() ? "UNKNOWN ERROR" : result;
    }

    // Helper to return one value from query
    template<typename T>
    T GetCell(const SQLite& database, const std::string& command, const T& defaultValue = T())
    {
        typedef SQLite::Row<T> Row_t;
        Row_t result = database.GetRow<Row_t>(command);
        if (result.bValid)
        {
            return result.Result;
        }

        return defaultValue;
    }


    //-----------------------------------------------------------------------------------------------------------------------------------------------
    namespace internal
    {
        // Converters are used when querying data and wa want to convert from string to appropriate
        //template<typename T> struct Convertor;

        //template<> struct Convertor<unused_marker_t>
        //{
        //    static unused_marker_t FromString(const char* value) { return unused_marker_t(); }
        //    static std::string ToString(unused_marker_t value) { return ""; }
        //};

        //template<> struct Convertor<const char *>
        //{
        //    static const char *FromString(const char* value) { return value; }
        //    static std::string ToString(const char *value) { return std::string(value); }
        //};

        //template<> struct Convertor<char>
        //{
        //    static char FromString(const char* value) { return value ? value[0] : 0; }
        //    static std::string ToString(char value) { char ReturnedValue[2]; ReturnedValue[0] = value; ReturnedValue[1] = '\0'; return ReturnedValue; }
        //};

        //template<> struct Convertor<int>
        //{
        //    static int FromString(const char* value) { return value ? atoi(value) : 0; }
        //    static std::string ToString(int value) { std::string retValue; retValue.Format("%d", value); return retValue; }
        //};

        //template<> struct Convertor<uint32_t>
        //{
        //    static uint32_t FromString(const char* value) { return value ? atoi(value) : 0; }
        //    static std::string ToString(uint32_t value) { std::string retValue; retValue.Format("%d", value); return retValue; }
        //};

        //template<> struct Convertor<bool>
        //{
        //    static bool FromString(const char* value) { return value ? value != std::string("0") : false; }
        //    static std::string ToString(bool value) { return value ? "1" : "0"; }
        //};

        //template<> struct Convertor<float>
        //{
        //    static float FromString(const char* value) { return value ? static_cast<float>(atof(value)) : 0.0f; }
        //    static std::string ToString(float value) { std::string retValue; retValue.Format("%f", value); return retValue; }
        //};

        //template<> struct Convertor<std::string>
        //{
        //    static std::string FromString(const char* value) { return value; }
        //    static std::string ToString(const std::string& value) { return value; }
        //};

        //template<> struct Convertor<SimpleVec3>
        //{
        //    static Vec3 FromString(const char* value) { Vec3 converted; LYGame::FromString(value, converted); return converted; }
        //    static std::string ToString(const Vec3& value) { return LYGame::ToString(value); }
        //};

        //template<> struct Convertor<Quat>
        //{
        //    static Quat FromString(const char* value) { Quat converted; LYGame::FromString(value, converted); return converted; }
        //    static string ToString(const Quat& value) { return LYGame::ToString(value); }
        //};

        template<typename T>
        struct CellCallback : SQLite::QueryCallback
        {
            virtual int operator()(int numValues, char **values, char **columnNames)
            {
                YAGET_ASSERT(numValues == 1, "QLite::QueryCallback mismatch of parameters");
                Value = conv::Convertor<T>::FromString(values[0]);
                return -1;
            }

            T Value;
        };

        template<typename RT>
        struct RowCallback : SQLite::QueryCallback
        {
            typedef RT Row_t;

            virtual int operator()(int numValues, char **values, char** /*columnNames*/)
            {
                static int numResults = Row_t::NumDim();
                YAGET_ASSERT(numValues == numResults, "Mismatch of num values. Incoming '%d', templetized '%d'", numValues, numResults);

                Row_t Value; Value;

                switch (numResults)
                {
                case 7:
                    Value.Result6 = conv::Convertor<Row_t::T6>::FromString(values[6]);
                case 6:
                    Value.Result5 = conv::Convertor<Row_t::T5>::FromString(values[5]);
                case 5:
                    Value.Result4 = conv::Convertor<Row_t::T4>::FromString(values[4]);
                case 4:
                    Value.Result3 = conv::Convertor<Row_t::T3>::FromString(values[3]);
                case 3:
                    Value.Result2 = conv::Convertor<Row_t::T2>::FromString(values[2]);
                case 2:
                    Value.Result1 = conv::Convertor<Row_t::T1>::FromString(values[1]);
                case 1:
                    Value.Result = conv::Convertor<Row_t::T>::FromString(values[0]);
                }

                Results.push_back(Value);

                return bMultipleRows ? 0 : - 1;

                //numValues; values;
                //return 0;
            }

            bool bMultipleRows = false;
            std::vector<Row_t> Results;
        };

    } // namespace internal

    //insert or replace into Groups(Name, Path, Filters, Converters, ReadOnly) values('FooFolder2', '$(Foo)', '*.sqlite', '', 1);
    template<typename RT>
    bool SQLite::ExecuteStatement(const StatementId_t& statementId, const std::string& tableName, const RT& dataValues, Behaviour automaticTime, Behaviour behaviour)
    {
        YAGET_ASSERT(mDatabase, "SQLite::ExecuteStatement<RT>: '%s' called for table: '%s', but sqlite db is not created yet.", statementId.c_str(), tableName.c_str());

        Statement* statement = nullptr;
        auto it = m_statements.find(statementId);
        if (it == m_statements.end())
        {
            int numValues = RT::NumDim() - 1;
            const char* updateStr = behaviour == Behaviour::Update ? "OR REPLACE" : "";
            std::string command = fmt::format("INSERT {} INTO '{}' VALUES (?", updateStr, tableName);
            while (numValues)
            {
                command += ", ?";
                numValues--;
            }

            if (automaticTime == Behaviour::TimeStampYes)
            {
                command += ", TIME('now', 'localtime')";
            }

            command += ");";
            m_statements[statementId] = Statement(mDatabase, command);
            statement = &m_statements[statementId];
        }
        else
        {
            statement = &it->second;
        }

        statement->UpdateStatement(dataValues);
        return ExecuteStatement(statement);
    }

    //-----------------------------------------------------------------------------------------------------------------------------------------------
    template<typename RT>
    RT SQLite::GetRow(const std::string& command) const
    {
        typedef RT Row_t;

        internal::RowCallback<Row_t> row; row; command;
        if (const_cast<SQLite&>(*this).ExecuteStatement(command, &row))
        {
            if (!row.Results.empty())
            {
                return row.Results[0];
            }
        }
        else
        {
            //CryLogAlways("Error: Could not read row. %s.", m_errorMsg.c_str());
            YAGET_ASSERT(true);
        }

        Row_t emptyRow;
        emptyRow.bValid = false;
        return emptyRow;
    }

    //-----------------------------------------------------------------------------------------------------------------------------------------------
    template<typename RT>
    std::vector<RT> SQLite::GetRows(const std::string& command) const
    {
        typedef RT Row_t;

        internal::RowCallback<Row_t> row;
        row.bMultipleRows = true;
        if (const_cast<SQLite&>(*this).ExecuteStatement(command, &row))
        {
            return row.Results;
        }
        else
        {
            return std::vector<Row_t>();
        }
    }

    template <typename R, typename Q, typename C>
    C SQLite::GetRows(const std::string& command, std::function<R(Q)> converter) const
    {
        std::vector<Q> dbResults = GetRows<Q>(command);
        C container;

        std::transform(dbResults.begin(), dbResults.end(), std::inserter(container, container.end()), [converter](const Q& record)
        {
            return converter(record);
        });

        return container;
    }


    //template <typename R, typename Q, typename C = std::vector<R>>
    //struct GetRows
    //{
    //    typename typedef R Result;
    //    typename typedef Q Query;
    //    typename typedef C Container;

    //    static Container db_GetRows(Database& database, const char* statement, std::function<Result(Query)> converter)
    //    {
    //        std::vector<Query> dbResults = database.DB().GetRows<Query>(statement);
    //        Container results;
    //        for (const auto& it : dbResults)
    //        {
    //            results.insert(converter(it));
    //        }

    //        return results;
    //    }
    //};


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
