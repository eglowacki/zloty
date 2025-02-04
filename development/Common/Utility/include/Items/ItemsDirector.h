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
    class Director : public Noncopyable<Director>
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

        // save T::mDataStorage to DB
        template <typename T>
        bool SaveComponentState(const T* component);

        // load T::Types from DB. If load not valid, result [out] value will be set to false
        // and it will return default T::Types{}.
        template <typename T>
        typename T::Types LoadComponentState(comp::Id_t id, bool* result) const;

        // Return list of item id's that are marked for stageName. This does not load/create items.
        comp::ItemIds GetStageItems(const std::string& stageName) const;
        void AddStageItems(const std::string& stageName, const comp::ItemIds& ids);
        void AddStageItem(const std::string& stageName, comp::Id_t id) { AddStageItems(stageName, {id}); }

        void RemoveStageItems(const std::string& stageName, const comp::ItemIds& ids);
        void RemoveStageItem(const std::string& stageName, comp::Id_t id) { RemoveStageItems(stageName, {id}); }

        int AddStage(const std::string& stageName);

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

    // Provides unified naming convention for storing db location, taking CoordinatorSet type to generate initial schema.
    template <typename T>
    class DefaultDirector : public Director
    {
    public:
        DefaultDirector(io::VirtualTransportSystem& vts, const std::string& name = "Director", RuntimeMode runTimeMode = RuntimeMode::Default)
            : Director("$(DatabaseFolder)/" + name + ".sqlite", comp::db::GenerateSystemsCoordinatorSchema<T>(), comp::db::GenerateDirectorLoadout<T>(vts, "Settings@" + name), comp::db::GenerateSystemsCoordinatorVersion<T>(), runTimeMode)
        {}

        DefaultDirector(const std::string& name = "Director", RuntimeMode runTimeMode = RuntimeMode::Default)
            : Director("$(DatabaseFolder)/" + name + ".sqlite", comp::db::GenerateSystemsCoordinatorSchema<T>(), {}, comp::db::GenerateSystemsCoordinatorVersion<T>(), runTimeMode)
        {}
    };

    // Just base support for id cache and version. 
    using BlankDefaultDirector = DefaultDirector<comp::db::EmptySchema>;

    template <typename T>
    bool Director::SaveComponentState(const T* component)
    {
        using ParameterNames = typename comp::db::RowDescription_t<T>::Row;
        using ParameterPack = typename comp::db::RowDescription_t<T>::Types;

        const auto tableName = comp::db::ResolveName<T>();
        Strings columnNames;
        columnNames.push_back("Id");
        columnNames.append_range(comp::db::GetPolicyRowNames<ParameterNames>());

        const SQLite::StatementId_t statementId = "statementId_" + tableName;

        auto componentId = static_cast<comp::Id_t>(component->Id());
        auto inputParam = std::tuple_cat(std::tuple(componentId), component->mDataStorage);

        const auto dbLock = LockDatabaseAccess();
        auto& database = dbLock->DB();
        db::Transaction transaction(database);

        auto result = database.ExecuteStatementTuple(statementId, tableName, inputParam, columnNames, SQLite::Behaviour::Update);
        if (!result)
        {
            transaction.Rollback();

            const std::string message = fmt::format("Updating component: '{}' failed. {}.", tableName, ParseErrors(database));
            YLOG_WARNING("DIRE", message.c_str());
        }

        return result;
    }

    template <typename T>
    typename T::Types Director::LoadComponentState(comp::Id_t id, bool* result) const
    {
        using Parameters = typename T::Types;
        using ParameterNames = typename comp::db::RowDescription_t<T>::Row;

        const auto parameterNames = comp::db::GetPolicyRowNames<ParameterNames>();
        const auto tableName = comp::db::ResolveName<T>();
        Parameters parameters{};

        const std::string command = fmt::format("SELECT {} FROM {} WHERE Id = '{}'", conv::Combine(parameterNames, ", "), tableName, static_cast<int64_t>(id));

        const auto dbLock = LockDatabaseAccess();
        const auto& database = dbLock->DB();

        bool getResult = true;
        const auto componentParameters = database.GetRowTuple<Parameters>(command, &getResult);
        if (result)
        {
            *result = getResult;
        }

        return componentParameters;
    }

} // namespace yaget::items


