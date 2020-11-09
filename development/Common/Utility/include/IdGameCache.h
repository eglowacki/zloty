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
#include <atomic>


namespace yaget
{
    namespace items { class Director; }

    class IdGameCache
    {
    public:
        enum class IdType {Burnable, Persistent};

        IdGameCache(items::Director* director);
        ~IdGameCache();

        //! Return next available id which will be burnable or persistent based on IdType parameter
        comp::Id_t GetId(IdType idType);

    private:
        //! Burnable id's will only fall within this range
        //! first <= currentId < second
        std::pair<comp::Id_t, comp::Id_t> mBurnableRange;
        //! next valid burnable id
        std::atomic_uint64_t mNextBurnableId;
        //! Next persistent id
        //! \note we need to get batch of id's from DB
        std::atomic_uint64_t mNextPersistentId;
        std::pair<comp::Id_t, comp::Id_t> mPersistentRange;

        //! If not null, then it's used for persistent queries
        items::Director* mDirector;
    };

    namespace idspace
    {
        //! Convenient functions to get id's
        comp::Id_t get_burnable(IdGameCache& idCache);
        comp::Id_t get_persistent(IdGameCache& idCache);

    } // namespace idspace
} // namespace yaget
