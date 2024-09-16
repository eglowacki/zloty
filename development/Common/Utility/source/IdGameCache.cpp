#include "IdGameCache.h"
#include "Debugging/Assert.h"
#include "Items/ItemsDirector.h"
#include "ThreadModel/JobPool.h"

namespace
{
    const yaget::comp::Id_t kBurnableNexId = 1000;
    const yaget::comp::Id_t kMaxRange = 1000000000;
    const yaget::comp::Id_t kPersistentNexId = kBurnableNexId;// +kMaxRange;

} // namespace

using namespace yaget;
IdGameCache::IdGameCache(const GetNextBatch& getNextBatch)
    : mBurnableRange({ kBurnableNexId, kMaxRange })
    , mNextBurnableId(kBurnableNexId)
    , mNextPersistentId(kPersistentNexId)
    , mPersistentRange({ kPersistentNexId, kMaxRange })
    , mGetNextBatch(getNextBatch)
{
    if (mGetNextBatch)
    {
        mJob = std::make_unique<mt::JobPool>("IdCache", 1);
        mPersistentRange = mGetNextBatch();
        mNextPersistentId = mPersistentRange.mNextId;
        mNextAvailablePersistentRange = mGetNextBatch();

        YLOG_INFO("IDS", "Batch of id's with '%s' entries allocated, starting id: '%s'.", conv::ToThousandsSep(mPersistentRange.mBatchSize).c_str(), conv::ToThousandsSep(mPersistentRange.mNextId).c_str());
        YLOG_INFO("IDS", "Batch of Next Available id's with '%s' entries allocated, starting id: '%s'.", conv::ToThousandsSep(mNextAvailablePersistentRange.mBatchSize).c_str(), conv::ToThousandsSep(mNextAvailablePersistentRange.mNextId).c_str());
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
        if (mGetNextBatch)
        {
            // are we run-out of id's in current batch
            if (!mPersistentRange.IsIdValid(mNextPersistentId))
            {
                mJob->Join();
                mPersistentRange = mNextAvailablePersistentRange;
                mNextAvailablePersistentRange = {};
                mJob->AddTask([this]()
                {
                    mNextAvailablePersistentRange = mGetNextBatch();
                });

                mNextPersistentId = mPersistentRange.mNextId;
                YLOG_INFO("IDS", "Batch of id's with '%s' entries allocated, starting id: '%s'.", conv::ToThousandsSep(mPersistentRange.mBatchSize).c_str(), conv::ToThousandsSep(mPersistentRange.mNextId).c_str());
            }
        }

        result = mNextPersistentId++;
        result = comp::MarkAsPersistent(result);
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


