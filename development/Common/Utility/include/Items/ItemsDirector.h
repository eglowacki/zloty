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

namespace yaget
{
    class Application;

    namespace items
    {
        //-------------------------------------------------------------------------------------------------
        // Manages overall scene like items-component, provides id cache wired to DB to assure unique
        // persistent id's across runs of application, unless DB is deleted.
        class Director : public Noncopyable<Director>
        {
        public:
            constexpr static int BatchIdMarker = 1;

            //! Default    - default values
            //! Optimum    - optimized, used in runtime game (NOT USED)
            //! Diagnostic - report diagnostic health of vts data (NOT USED)
            //! Fix        - fix any vts data issues (NOT USED)
            //! Reset      - reset vts data and initialize as new 
            enum class RuntimeMode { Default, Optimum, Diagnostic, Fix, Reset };


            Director(const std::string& name, const Strings& additionalSchema, const Strings& loadout, int64_t expectedVersion, RuntimeMode runtimeMode);
            virtual ~Director();

            IdGameCache& IdCache() { return mIdGameCache; }

        private:
            IdBatch GetNextBatch();

            //--------------------------------------------------------------------------------------------------
            // provides locking for DB for read/write, use LockDatabaseAccess() accessors to acquire one
            struct DatabaseLocker
            {
                DatabaseLocker(Director& director) : mDatabase(director.mDatabase) {}
                virtual ~DatabaseLocker() = default;

                const SQLite& DB() const { return mDatabase.DB(); }
                SQLite& DB() { return mDatabase.DB(); }

            private:
                Database& mDatabase;
            };
            using DatabaseHandle = std::unique_ptr<DatabaseLocker>;

            struct Locker : public DatabaseLocker
            {
                Locker(std::mutex& mutex, Director& director) : DatabaseLocker(director), mMutex(mutex) { mMutex.lock(); }
                ~Locker() override { mMutex.unlock(); }

            private:
                std::mutex& mMutex;
            };

            virtual DatabaseHandle LockDatabaseAccess() { return std::make_unique<Locker>(mDatabaseMutex, *this); }
            virtual DatabaseHandle LockDatabaseAccess() const { return std::make_unique<Locker>(mDatabaseMutex, const_cast<Director&>(*this)); }

            Database mDatabase;
            mutable std::mutex mDatabaseMutex;
            IdGameCache mIdGameCache;
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

    } // namespace items
} // namespace yaget


