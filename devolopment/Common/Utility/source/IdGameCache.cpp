#include "IdGameCache.h"
#include "Debugging/Assert.h"
#include "Items/ItemsDirector.h"

namespace
{
    const uint64_t kBurnableMinRange = 1000;
    const uint64_t kBurnableMaxRange = 1000000000;

} // namespace

using namespace yaget;
IdGameCache::IdGameCache(items::Director* director)
    : mBurnableRange(kBurnableMinRange, kBurnableMaxRange)
    , mNextBurnableId(kBurnableMinRange)
    , mNextPersistentId(kBurnableMaxRange)
    , mDirector(director)
{
    if (mDirector)
    {
        items::IdBatch idBatch = mDirector->GetNextBatch();
        if (idBatch != items::IdBatch{})
        {
            mNextPersistentId = idBatch.mNextId;
            mPersistentRange = std::make_pair(idBatch.mNextId, idBatch.mBatchSize);

            YLOG_INFO("IDS", "New Id Batch from: '%s' with '%llu' entries.", conv::ToThousandsSep(mPersistentRange.first).c_str(), mPersistentRange.second);
        }
    }
}


IdGameCache::~IdGameCache()
{
}


uint64_t IdGameCache::GetId(IdType idType)
{
    uint64_t result = 0;
    if (idType == IdType::itBurnable)
    {
        YAGET_ASSERT(mNextBurnableId < mBurnableRange.second, "No more burnable id's available");
        result = mNextBurnableId++;
    }
    else if (idType == IdType::itPersistent)
    {
        if (mDirector)
        {
            const uint64_t topRange = mPersistentRange.first + mPersistentRange.second;
            if (mNextPersistentId == topRange)
            {
                // we are out of id's, get some more. This will be blocking call
                items::IdBatch idBatch = mDirector->GetNextBatch();
                YAGET_ASSERT(idBatch != items::IdBatch{}, "Next batch of Id's is not valid. Got empty set.");

                mNextPersistentId = idBatch.mNextId;
                mPersistentRange = std::make_pair(idBatch.mNextId, idBatch.mBatchSize);
                YLOG_INFO("IDS", "New Id Batch from: '%s' with '%llu' entries.", conv::ToThousandsSep(mPersistentRange.first).c_str(), mPersistentRange.second);
            }

            // if we are 'close' to exhaustion of id, we need to request more from director
        }

        result = mNextPersistentId++;
    }

    return result;
}


uint64_t idspace::get_burnable(IdGameCache& idCache)
{
    return idCache.GetId(IdGameCache::IdType::itBurnable);
}


uint64_t idspace::get_persistent(IdGameCache& idCache)
{
    return idCache.GetId(IdGameCache::IdType::itPersistent);
}


