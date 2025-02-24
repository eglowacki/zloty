/////////////////////////////////////////////////////////////////////
// IdGameCache.h
//
//  Copyright 1/14/2008 Edgar Glowacki.
//
//  Maintained by: Edgar
//
//  NOTES:
//      Provides id's for persistent and burnable id's
//      Persistent id's are extracted from DB, and burnable
//      are created locally which fall within some specified range which will
//      not overlap with persistent ones.
//
//
//  #include "IdGameCache.h"
//
//////////////////////////////////////////////////////////////////////
//! \file
#pragma once

#include "Components/ComponentTypes.h"
#include <functional>
#include <atomic>


namespace yaget
{
    namespace mt { class JobPool; }

    class IdGameCache
    {
    public:
        // if we need to operate in persistent ids (backed by DB), pass
        // callback of this signature to ctor, and return new batches from it
        using GetNextBatch = std::function<items::IdBatch()>;

        enum class IdType {Burnable, Persistent};

        IdGameCache(const GetNextBatch& getNextBatch);
        ~IdGameCache() = default;

        //! Return next available id which will be burnable or persistent based on IdType parameter
        comp::Id_t GetId(IdType idType);

    private:
        //! Burnable id's will only fall within this range
        //! first <= currentId < second
        items::IdBatch mBurnableRange;
        //! next valid burnable id
        std::atomic_int64_t mNextBurnableId;
        //! Next persistent id
        std::atomic_int64_t mNextPersistentId;
        // this keep track of current persistent range and get id's from it
        // when we run out of id's, we switch with Next and request new batch async
        items::IdBatch mPersistentRange;
        items::IdBatch mNextAvailablePersistentRange;

        //! If not null, then it's used for persistent queries
        GetNextBatch mGetNextBatch;
        std::unique_ptr<mt::JobPool> mJob;
    };

    namespace idspace
    {
        //! Convenient functions to get id's
        comp::Id_t get_burnable(IdGameCache& idCache);
        comp::Id_t get_persistent(IdGameCache& idCache);

    } // namespace idspace
} // namespace yaget
