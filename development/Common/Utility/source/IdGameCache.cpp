#include "IdGameCache.h"
#include "Debugging/Assert.h"
#include "Items/ItemsDirector.h"

namespace
{
    const yaget::comp::Id_t kBurnableMinRange = 1000;
    const yaget::comp::Id_t kBurnableMaxRange = 1000000000;

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
        YAGET_ASSERT(idBatch != items::IdBatch{}, "Initial batch of Id's is not valid. Got empty set.");
        mNextPersistentId = idBatch.mNextId;
        mPersistentRange = std::make_pair(idBatch.mNextId, idBatch.mBatchSize);

        YLOG_INFO("IDS", "New Id Batch from: '%s' with '%llu' entries.", conv::ToThousandsSep(mPersistentRange.first).c_str(), mPersistentRange.second);
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
        YAGET_ASSERT(mNextBurnableId < mBurnableRange.second, "No more burnable id's available");
        result = mNextBurnableId++;
    }
    else if (idType == IdType::Persistent)
    {
        if (mDirector)
        {
            const auto topRange = mPersistentRange.first + mPersistentRange.second;
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


comp::Id_t idspace::get_burnable(IdGameCache& idCache)
{
    return idCache.GetId(IdGameCache::IdType::Burnable);
}


comp::Id_t idspace::get_persistent(IdGameCache& idCache)
{
    return idCache.GetId(IdGameCache::IdType::Persistent);
}


