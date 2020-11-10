#include "IdGameCache.h"
#include "Debugging/Assert.h"
#include "Items/ItemsDirector.h"

namespace
{
    const yaget::comp::Id_t kBurnableNexId = 1000;
    const yaget::comp::Id_t kMaxRange = 1000000000;
    const yaget::comp::Id_t kPersistentNexId = kBurnableNexId + kMaxRange;

} // namespace

using namespace yaget;
IdGameCache::IdGameCache(items::Director* director)
    : mBurnableRange({ kBurnableNexId, kMaxRange })
    , mNextBurnableId(kBurnableNexId)
    , mNextPersistentId(kPersistentNexId)
    , mPersistentRange({ kPersistentNexId, kMaxRange })
    , mDirector(director)
{
    if (mDirector)
    {
        mPersistentRange = mDirector->GetNextBatch();
        mNextPersistentId = mPersistentRange.mNextId;
        mNextAvailablePersistentRange = mDirector->GetNextBatch();

        YLOG_INFO("IDS", "New Id Batch from: '%s' with '%llu' entries.", conv::ToThousandsSep(mPersistentRange.mNextId).c_str(), mPersistentRange.mBatchSize);
    }
}


IdGameCache::~IdGameCache()
{
}


comp::Id_t IdGameCache::GetId(IdType idType)
{
    comp::Id_t result = 0;
    if (idType == IdType::Burnable)
    {
        YAGET_ASSERT(mBurnableRange.IsIdValid(mNextBurnableId), "No more burnable id's available");
        result = mNextBurnableId++;
    }
    else if (idType == IdType::Persistent)
    {
        if (mDirector)
        {
            if (!mPersistentRange.IsIdValid(mNextPersistentId))
            {
                if (mNextAvailablePersistentRange != items::IdBatch{})
                {
                    mPersistentRange = mNextAvailablePersistentRange;
                    mNextAvailablePersistentRange = {};
                    // we have valid next batch
                }
                else
                {
                    // this really should not have happen, since we should have gotten
                    // callback from last request for next id batch.
                    // we are out of id's, get some more. This will be blocking call
                    mPersistentRange = mDirector->GetNextBatch();
                }

                mNextPersistentId = mPersistentRange.mNextId;
                YLOG_INFO("IDS", "New Id Batch from: '%s' with '%llu' entries.", conv::ToThousandsSep(mPersistentRange.mNextId).c_str(), mPersistentRange.mBatchSize);
            }

            // if we are 'close' to exhaustion of id, we need to request more from director
        }

        result = mNextPersistentId++;
    }

    return comp::MarkAsPersistent(result);
}


comp::Id_t idspace::get_burnable(IdGameCache& idCache)
{
    return idCache.GetId(IdGameCache::IdType::Burnable);
}


comp::Id_t idspace::get_persistent(IdGameCache& idCache)
{
    return idCache.GetId(IdGameCache::IdType::Persistent);
}


