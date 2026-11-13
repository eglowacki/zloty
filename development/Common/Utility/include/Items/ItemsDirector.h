///////////////////////////////////////////////////////////////////////
// ItemDatabase.h
//
//  Copyright 7/31/2016 Edgar Glowacki.
//
//  Maintained by: Edgar
//
//  NOTES:
//      Storage and management of items in a game. This uses sqlite on the back end
//
//
//  #include "Items/ItemsDirector.h"
//
//////////////////////////////////////////////////////////////////////
//! \file
#pragma once

#include "YagetCore.h"
#include "Database/Database.h"
#include "Components/GameCoordinatorGenerator.h"
#include "IdGameCache.h"

namespace yaget::items
{
    //-------------------------------------------------------------------------------------------------
    // Manages overall scene like items-component, provides id cache wired to DB to assure unique
    // persistent id's across runs of application, unless DB is deleted.
    class Director : public NoCopy
    {
    public:
        constexpr static int BatchIdMarker = 1;
        constexpr static int InvalidStageId = 0;

        //! Default    - default values
        //! Optimum    - optimized, used in runtime game (NOT USED)
        //! Diagnostic - report diagnostic health of director data (NOT USED)
        //! Fix        - fix any director data issues (NOT USED)
        //! Reset      - reset director data and initialize as new 
        enum class RuntimeMode { Default, Optimum, Diagnostic, Fix, Reset };

        Director(const std::string& name, const Strings& additionalSchema, const Strings& loadout, int64_t expectedVersion, RuntimeMode runtimeMode);
        virtual ~Director();

        IdGameCache& IdCache() { return mIdGameCache; }

        template <typename ComponentParams>
        bool SaveComponentState(const std::string& componentName, const ComponentParams& inputParam);

        template <typename S>
        bool SaveComponentState(const std::string& componentName, comp::Id_t componentId, const S& storage);

        // save T::mDataStorage to DB
        template <typename T>
        bool SaveComponentState(const T* component);

        // load T::Types from DB. If load not valid, result [out] value will be set to false
        // and it will return default T::Types{}.
        template <typename T>
        comp::db::RowDescription_t<T>::Types LoadComponentState(comp::Id_t id, bool* result) const;

        // Return list of item id's that are marked for stageName. This does not load/create items.
        comp::ItemIds GetStageItems(const std::string& stageName) const;
        void AddStageItems(const std::string& stageName, const comp::ItemIds& ids);
        void AddStageItem(const std::string& stageName, comp::Id_t id) { AddStageItems(stageName, {id}); }

        void RemoveStageItems(const std::string& stageName, const comp::ItemIds& ids);
        void RemoveStageItem(const std::string& stageName, comp::Id_t id) { RemoveStageItems(stageName, {id}); }

        int AddStage(const std::string& stageName);

        template <typename R, typename P>
        bool AddUserFunction(const std::string& functionName, SQLite::UserFunction<R, P> userFunction);

    private:
        IdBatch GetNextBatch();

        //--------------------------------------------------------------------------------------------------
        // provides locking for DB for read/write, use LockDatabaseAccess() accessors to acquire one
        struct DatabaseLocker
        {
            DatabaseLocker(SQLite& database) : mDatabase(database) {}
            virtual ~DatabaseLocker() = default;

            const SQLite& DB() const { return mDatabase; }
            SQLite& DB() { return mDatabase; }

        private:
            SQLite& mDatabase;
        };
        using DatabaseHandle = std::unique_ptr<DatabaseLocker>;

        struct Locker : public DatabaseLocker
        {
            Locker(std::mutex& mutex, SQLite& database) : DatabaseLocker(database), mMutex(mutex) { mMutex.lock(); }
            ~Locker() override { mMutex.unlock(); }

        private:
            std::mutex& mMutex;
        };

        virtual DatabaseHandle LockDatabaseAccess() { return std::make_unique<Locker>(mDatabaseMutex, mDatabase.DB()); }
        virtual DatabaseHandle LockDatabaseAccess() const { return std::make_unique<Locker>(mDatabaseMutex, const_cast<SQLite&>(mDatabase.DB())); }

        void CacheStageNames();
        int GetStageId(const std::string& stageName) const;

        Database mDatabase;
        mutable std::mutex mDatabaseMutex;
        IdGameCache mIdGameCache;

        using StageName = std::tuple<std::string, int>;
        using StageNames = std::vector<StageName>;

        StageNames mStageNames;
    };


    //--------------------------------------------------------------------------------------------------
    // Provides unified naming convention for storing db location, taking CoordinatorSet type to generate initial schema.
    template <typename T>
    class DefaultDirector : public Director
    {
    public:
        DefaultDirector(const std::string& name = "Director", RuntimeMode runTimeMode = RuntimeMode::Default)
            : Director("$(DatabaseFolder)/" + name + ".sqlite", comp::db::GenerateSystemsCoordinator<T, comp::db::GenerateCoordinator::Schema>(), {}, comp::db::GenerateSystemsCoordinator<T, comp::db::GenerateCoordinator::Version>(), runTimeMode)
        {}
    };


    //--------------------------------------------------------------------------------------------------
    template <typename T>
    class SaveDirector : public Director
    {
    public:
        SaveDirector(const std::string& name, RuntimeMode runTimeMode = RuntimeMode::Default)
            : Director("$(SaveDataFolder)/" + name + ".sqlite", comp::db::GenerateSystemsCoordinator<T, comp::db::GenerateCoordinator::Schema>(), {}, comp::db::GenerateSystemsCoordinator<T, comp::db::GenerateCoordinator::Version>(), runTimeMode)
        {}
    };


    //--------------------------------------------------------------------------------------------------
    template <typename T>
    class SetupDirector : public DefaultDirector<T>
    {
    public:
        SetupDirector(const std::string& name = "Director")
            : DefaultDirector<T>(name, Director::RuntimeMode::Reset)
        {}
    };

    // Just base support for id cache and version, using pre-existing
    // database file if exists, thus preserving db data
    using DefaultBlankDirector = DefaultDirector<comp::db::EmptySchema>;

    // Just base support for id cache and version, using new
    // database file, thus destroying previous data
    using SetupBlankDirector = SetupDirector<comp::db::EmptySchema>;


    //--------------------------------------------------------------------------------------------------
    template <typename ComponentParams>
    bool Director::SaveComponentState(const std::string& componentName, const ComponentParams& inputParam)
    {
        const SQLite::StatementId_t statementId = "statementId_" + componentName;

        const auto dbLock = LockDatabaseAccess();
        auto& database = dbLock->DB();
        db::Transaction transaction(database);

        // SELECT 1 FROM PRAGMA_TABLE_INFO('your_table') WHERE name='column1';
        // SELECT * FROM PRAGMA_TABLE_INFO('LocationComponent3');
        //      cid name        type    notnull dflt_value  pk
        //      0	Id	        INTEGER	0		NULL        1
        //      1	Position	STRING	0		NULL        0
        //      2	Orientation	STRING	0		NULL        0
        //      3	Scale	    STRING	0		NULL        0
        auto result = database.ExecuteStatementTuple(statementId, componentName, inputParam, {}, SQLite::Behaviour::Update);
        if (!result)
        {
            transaction.Rollback();

            const std::string message = std::format("Updating component: '{}' failed. {}.", componentName, ParseErrors(database));
            YLOG_WARNING("DIRE", message.c_str());
        }

        return result;
    }


    //--------------------------------------------------------------------------------------------------
    template <typename S>
    bool Director::SaveComponentState(const std::string& componentName, comp::Id_t componentId, const S& storage)
    {
        auto inputParam = std::tuple_cat(std::tuple(componentId), storage);
        return SaveComponentState(componentName, inputParam);
    }


    //--------------------------------------------------------------------------------------------------
    template <typename T>
    bool Director::SaveComponentState(const T* component)
    {
        YLOG_CERROR("DIRE", (component != nullptr), "component parameter is nullptr. Can not save state of null object.");
        if (!component)
        {
            return false;
        }

        const auto componentName = comp::db::ResolveName<T>();
        return SaveComponentState(componentName, component->Id(), component->GetStorage());
    }


    //--------------------------------------------------------------------------------------------------
    template <typename T>
    typename comp::db::RowDescription_t<T>::Types Director::LoadComponentState(comp::Id_t id, bool* result) const
    {
        using Parameters = typename comp::db::RowDescription_t<T>::Types;
        using ParameterNames = typename comp::db::RowDescription_t<T>::Row;
        constexpr bool NoParameters = std::tuple_size_v<Parameters> == 0;
        const auto tableName = comp::db::ResolveName<T>();
        auto parameterNames = comp::db::GetPolicyRowNames<ParameterNames>();

        // if there are no parameters specified, we still want to generate a query to see if the record exists, 
        // so we will use COUNT(*) to check and then disregard results and return default Parameters{}.
        // This will not generate any errors, which is desired behavior, since we are just checking if record exists.
        if constexpr (NoParameters)
        {
            parameterNames.push_back("COUNT(*)");
        }

        const std::string command = std::format("SELECT {} FROM {} WHERE Id = '{}'", conv::Combine(parameterNames, ", "), tableName, static_cast<int64_t>(id));

        const auto dbLock = LockDatabaseAccess();
        const auto& database = dbLock->DB();

        if constexpr (NoParameters)
        {
            using Parameters = std::tuple<comp::Id_t>;
            auto queryResult = database.GetRowTuple<Parameters>(command, result);

            return {};
        }
        else
        {
            return database.GetRowTuple<Parameters>(command, result);
        }
    }


    //--------------------------------------------------------------------------------------------------
    template <typename R, typename P>
    bool Director::AddUserFunction(const std::string& functionName, SQLite::UserFunction<R, P> userFunction)
    {
        DatabaseHandle databaseHandle = LockDatabaseAccess();
        return databaseHandle->DB().AddUserFunction(functionName, userFunction);
    }


    //--------------------------------------------------------------------------------------------------
    template <typename SC>
    requires comp::metatypes::has_pattern_function<SC>
    void BindUserFunctions(SC& systemCoordinator)
    {
        using FullRow = comp::metatypes::coordinator_set_t<SC>::FullRow;
        using ItemPattern = std::bitset<std::tuple_size_v<FullRow>>;
        const Strings componentNames = comp::db::GetPolicyRowNames<FullRow>();

        using UserParameters = std::tuple<std::string /*componentName*/, std::string /*action*/, yaget::comp::Id_t /*id*/>;
        using UserFunction = SQLite::UserFunction<std::string, UserParameters>;
        
        UserFunction userFunction = [componentNames](const SQLite& database, const UserParameters& userParameters)
        {
            const auto& [componentName, action, id] = userParameters;
            const std::string command = std::format("SELECT Pattern FROM ItemPattern WHERE Id = {}", id);
            auto itemPattern = yaget::GetCell(database, command, std::string{});
            ItemPattern iPattern{ itemPattern };

            auto it = std::ranges::find(componentNames, componentName);
            size_t componentIndex = it - componentNames.begin();

            if (action == "ADD")
            {
                iPattern.set(componentIndex);
            }
            else if (action == "REMOVE")
            {
                iPattern.reset(componentIndex);
            }

            auto result = iPattern.to_string();

            return result;
        };

        auto patternFunctionName = comp::db::ResolveName<typename SC::PatternFunction>();
        auto& director = systemCoordinator.Director();
        director.AddUserFunction(patternFunctionName, userFunction);
    }


} // namespace yaget::items


