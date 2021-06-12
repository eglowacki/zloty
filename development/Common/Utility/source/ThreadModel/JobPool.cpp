#include "ThreadModel/JobPool.h"
#include "Debugging/DevConfiguration.h"
#include "Logger/YLog.h"
#include "ThreadModel/Variables.h"
#include "fmt/format.h"
#include <algorithm>



namespace
{
    class NameIndexer
    {
    public:
        std::string GetNextName(const std::string& name)
        {
            int value = 0;
            {
                std::lock_guard<std::mutex> mutexLock(mMutex);
                value = ++mCounters[name];
            }

            return GenerateName(name, value);
        }

    private:
        std::string GenerateName(const std::string& name, int index) const
        {
            return name + "." + yaget::conv::Convertor<int>::ToString(index);
        }

        using Counters = std::unordered_map<std::string, int>;
        Counters mCounters;

        mutable std::mutex mMutex;
    };

    NameIndexer& Indexer()
    {
        static NameIndexer nameIndexer;

        return nameIndexer;
    }

    void StopThreads(yaget::mt::JobPool::Threads_t& threads)
    {
        for (auto&& it : threads)
        {
            it.second.Clear();
        }
    }

    uint32_t CalculateMaxNumThreads(uint32_t numThreads)
    {
        uint32_t maxThreads = numThreads;
        if (numThreads == 0)
        {
            maxThreads = std::thread::hardware_concurrency() > 1 ? std::thread::hardware_concurrency() - 1 : 1;
        }

        return maxThreads;
    }

}


std::string yaget::mt::GenerateNextName(const std::string& name)
{
    return Indexer().GetNextName(name);
}


yaget::mt::JobPool::JobPool(const char* poolName, uint32_t numThreads /*= 0*/, Behaviour behaviour /*= Behaviour::StartAsRun*/) 
    : mName(GenerateNextName(poolName))
    , mBehaviour(behaviour)
    , mDynamicThreads(true)
    , mMaxNumThreads(CalculateMaxNumThreads(numThreads))
{
    mEmptyCondition.Trigger();

    YLOG_DEBUG("POOL", "Creating JobPool '%s' with '%d' threads.", mName.c_str(), mMaxNumThreads);

    const auto numThreadsToCreate = mDynamicThreads ? 0 : mMaxNumThreads;
    for (uint32_t i = 0; i < numThreadsToCreate; ++i)
    { 
        std::string threadName = mMaxNumThreads > 1 ? fmt::format("{}_{}/{}", mName, i + 1, mMaxNumThreads) : mName;
        mThreads.insert(std::make_pair(threadName, JobProcessor::Holder(threadName, [this]() { return PopNextTask(); }))); 
    } 
} 


void yaget::mt::JobPool::Clear() 
{
    {
        mt::unique_lock mutexLock = mDynamicThreads ? mt::unique_lock(mThreadListMutext) : mt::unique_lock{};
        for (auto&& it : mThreads)
        {
            it.second.Clear();
        }
    }

    mEmptyCondition.Trigger();
}


void yaget::mt::JobPool::Destroy()
{
    {
        std::unique_lock<std::mutex> mutexLock(mPendingTasksMutex);
        if (!mTasks.empty())
        {
            YLOG_DEBUG("POOL", "Deleting threads for JobPool '%s'.%s", mName.c_str(), (mTasks.empty() ? "" : fmt::format(" There are '{}' unfinished tasks in queue.", mTasks.size()).c_str()));
            mTasks.clear();
        }
    }

    Clear();

    mt::unique_lock mutexLock = mDynamicThreads ? mt::unique_lock(mThreadListMutext) : mt::unique_lock{};
    if (!mThreads.empty())
    {
        mThreads.clear();
        YLOG_DEBUG("POOL", "Done deleting threads for JobPool '%s'.", mName.c_str());
    }
}


yaget::mt::JobPool::~JobPool() 
{
    Destroy();
}


void yaget::mt::JobPool::UpdateThreadPool(size_t numTasksLeft)
{
    if (mDynamicThreads && numTasksLeft)
    {
        mt::unique_lock mutexLock = mt::unique_lock(mThreadListMutext);
        size_t threadListSize = mThreads.size();

        // looks like we have tasks left, let check to see if we reached man number of threads
        // and if not, are all busy?
        while (numTasksLeft && threadListSize < mMaxNumThreads)
        //if (threadListSize < mMaxNumThreads)
        {
            // https://app.gitkraken.com/glo/view/card/c09a1b0ebde34dfd96f0a8737d446677 (KAR-41)
            const bool allBusy = std::ranges::all_of(mThreads.begin(), mThreads.end(), [](auto itr)
            {
                return itr.second.IsBusy();
            });

            if (allBusy)
            {
                // spawn another thread
                std::string threadName = mMaxNumThreads > 1 ? fmt::format("{}_{}/{}", mName, threadListSize + 1, mMaxNumThreads) : mName;
                mThreads.insert(std::make_pair(threadName, JobProcessor::Holder(threadName, [this]() { return PopNextTask(); })));
            }

            threadListSize = mThreads.size();
            --numTasksLeft;
        }
    }
}


size_t yaget::mt::JobPool::GetNumTasksLeft()
{
    std::unique_lock<std::mutex> mutexLock(mPendingTasksMutex);
    const size_t numTasksLeft = mTasks.size();
    return numTasksLeft;
}


void yaget::mt::JobPool::AddTask(mt::JobProcessor::Task_t task) 
{
    size_t numTasksLeft = 0;
    mEmptyCondition.Reset();
    {
        std::unique_lock<std::mutex> mutexLock(mPendingTasksMutex);
        mTasks.push_back(task);
        numTasksLeft = mTasks.size();
    }

    if (mBehaviour == Behaviour::StartAsRun) 
    { 
        UpdateThreadPool(numTasksLeft);
        UnpauseAll();
    } 
} 


void yaget::mt::JobPool::Join() 
{ 
    mEmptyCondition.Wait(); 
    mEmptyCondition.Trigger();
}

void yaget::mt::JobPool::JoinDestroy()
{
    Join();
    Destroy();
}


void yaget::mt::JobPool::UnpauseAll() 
{
    if (mDynamicThreads && mBehaviour == Behaviour::StartAsPause)
    {
        const size_t numTasksLeft = GetNumTasksLeft();
        UpdateThreadPool(numTasksLeft);
    }

    mEmptyCondition.Reset();
    mBehaviour = Behaviour::StartAsRun;
 
    mt::unique_lock mutexLock = mDynamicThreads ? mt::unique_lock(mThreadListMutext) : mt::unique_lock{};
    for (auto&& it : mThreads)
    { 
        it.second.StartProcessing(); 
    } 
} 


yaget::mt::JobProcessor::Task_t yaget::mt::JobPool::PopNextTask()
{
    JobProcessor::Task_t task = {};
    {
        std::unique_lock<std::mutex> mutexLock(mPendingTasksMutex);
        if (!mTasks.empty())
        {
            task = *mTasks.begin();
            mTasks.pop_front();
        }
    }

    if (task)
    {
        return task;
    }
 
    mEmptyCondition.Trigger(); 
    return JobProcessor::Task_t(); 
} 
