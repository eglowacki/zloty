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
        struct IdBatch
        {
            uint64_t mNextId;
            uint64_t mBatchSize;
        };

        inline bool operator==(const IdBatch& lh, const IdBatch& rh) { return lh.mBatchSize == rh.mBatchSize && lh.mNextId == rh.mNextId; }
        inline bool operator!=(const IdBatch& lh, const IdBatch& rh) { return !(lh == rh); }

        //-------------------------------------------------------------------------------------------------
        // Manages overall scene like items-component, provides id cache wired to DB to assure unique
        // persistent id's across runs of application, unless DB is deleted.
        class Director : public Noncopyable<Director>
        {
        public:
            Director(const std::string& name, const Strings& additionalSchema, int64_t expectedVersion);
            virtual ~Director();

            IdBatch GetNextBatch();

            IdGameCache& IdCache() { return mIdGameCache; }

        private:
            //--------------------------------------------------------------------------------------------------
            // provides locking for DB for read/write, use LockDatabaseAccess() accessors to acquire one
            struct DatabaseLocker
            {
                DatabaseLocker(Director& director) : mDatabase(director.mDatabase) {}
                virtual ~DatabaseLocker() {}

                const SQLite& DB() const { return mDatabase.DB(); }
                SQLite& DB() { return mDatabase.DB(); }

            private:
                Database& mDatabase;
            };
            using DatabaseHandle = std::unique_ptr<DatabaseLocker>;

            struct Locker : public DatabaseLocker
            {
                Locker(std::mutex& mutex, Director& director) : DatabaseLocker(director), mMutex(mutex) { mMutex.lock(); }
                ~Locker() { mMutex.unlock(); }

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
            DefaultDirector()
                : Director("$(DatabaseFolder)/director.sqlite", comp::db::GenerateGameDirectorSchema<T>(mSchemaVersion), Database::NonVersioned)
            {}

        protected:
            DefaultDirector(const char* name)
                : Director("$(DatabaseFolder)/" + std::string(name) + std::string(".sqlite"), comp::db::GenerateGameDirectorSchema<T>(mSchemaVersion), Database::NonVersioned)
            {}

        private:
            inline static int64_t mSchemaVersion = 0;
        };

        // Just base support for id cache and version. 
        using BlankDefaultDirector = DefaultDirector<comp::db::EmptySchema>;

        // Exposes custom name for director sqlite file on disk. Location and ext are automaticly added
        template <typename T>
        class NamedDirector : public DefaultDirector<T>
        {
        public:
            NamedDirector(const char* name)
                : DefaultDirector(name)
            {}
        };

    } // namespace items
} // namespace yaget


