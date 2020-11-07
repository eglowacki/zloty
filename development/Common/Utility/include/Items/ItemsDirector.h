///////////////////////////////////////////////////////////////////////
// ItemDatabase.h
//
//  Copyright 7/31/2016 Edgar Glowacki.
//
//  Maintained by: Edgar
//
//  NOTES:
//      Storage and management of items in a game. This use sqlte on the back end
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
        class Director : public Noncopyable<Director>
        {
        public:
            Director(const std::string& name, const Strings& additionalSchema, int64_t expectedVersion);
            virtual ~Director();

            IdBatch GetNextBatch();
            SQLite& DB() { return mDatabase.DB(); }
            const SQLite& DB() const { return mDatabase.DB(); }

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

        template <typename T>
        class DefaultDirector : public Director
        {
        public:
            DefaultDirector()
                : Director("$(DatabaseFolder)/director.sqlite", comp::db::GenerateGameDirectorSchema<T>(mSchemaVersion), Database::NonVersioned)
            {}

        private:
            inline static int64_t mSchemaVersion = 0;
        };

        using BlankDefaultDirector = DefaultDirector<comp::db::EmptySchema>;

    } // namespace items
} // namespace yaget


