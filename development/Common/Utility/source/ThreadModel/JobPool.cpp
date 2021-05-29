#include "ThreadModel/JobPool.h" 
#include "Logger/YLog.h" 
#include "fmt/format.h" 
#include "Platform/Support.h" 
 
yaget::mt::JobPool::JobPool(const char* poolName, uint32_t numThreads /*= 0*/, Behaviour behaviour /*= Behaviour::StartAsRun*/) 
    : mName(poolName) 
    , mBehaviour(behaviour) 
{
    mEmptyCondition.Trigger();

    uint32_t maxThreads = numThreads; 
    if (numThreads == 0) 
    { 
        maxThreads = std::thread::hardware_concurrency() > 1 ? std::thread::hardware_concurrency() - 1 : 1; 
    } 
 
    YLOG_DEBUG("POOL", "Creating JobPool '%s' with '%d' threads.", mName.c_str(), maxThreads); 
 
    for (uint32_t i = 0; i < maxThreads; ++i) 
    { 
        std::string threadName = maxThreads > 1 ? fmt::format("{}_{}/{}", mName, i + 1, maxThreads) : mName;
        mThreads.insert(std::make_pair(threadName, JobProcessor::Holder(threadName, [this]() { return PopNextTask(); }))); 
    } 
} 
 
void yaget::mt::JobPool::Clear() 
{ 
    for (auto&& it : mThreads) 
    { 
        it.second.Clear(); 
    } 
    mEmptyCondition.Trigger();
}
 
 
yaget::mt::JobPool::~JobPool() 
{ 
    { 
        std::unique_lock<std::mutex> mutexLock(mPendingTasksMutex); 
        YLOG_DEBUG("POOL", "Deleting threads for JobPool '%s'.%s", mName.c_str(), (mTasks.empty() ? "" : fmt::format(" There are '{}' unfinished tasks in queue.", mTasks.size()).c_str())); 
        mTasks.clear(); 
    } 
 
    Clear(); 
    mThreads.clear(); 
    YLOG_DEBUG("POOL", "Done deleting threads for JobPool '%s'.", mName.c_str()); 
} 
 
void yaget::mt::JobPool::AddTask(mt::JobProcessor::Task_t task) 
{
    mEmptyCondition.Reset();
    { 
        std::unique_lock<std::mutex> mutexLock(mPendingTasksMutex); 
        mTasks.push_back(task); 
    } 
 
    if (mBehaviour == Behaviour::StartAsRun) 
    { 
        UnpauseAll(); 
    } 
} 
 
void yaget::mt::JobPool::Join() 
{ 
    mEmptyCondition.Wait(); 
    mEmptyCondition.Trigger();
}
 
void yaget::mt::JobPool::UnpauseAll() 
{ 
    mEmptyCondition.Reset();
    mBehaviour = Behaviour::StartAsRun;
 
    for (auto&& it : mThreads) 
    { 
        it.second.StartProcessing(); 
    } 
} 
 
yaget::mt::JobProcessor::Task_t yaget::mt::JobPool::PopNextTask()
{
    std::unique_lock<std::mutex> mutexLock(mPendingTasksMutex);
    if (!mTasks.empty())
    { 
        JobProcessor::Task_t task = *mTasks.begin();
        mTasks.pop_front();
 
        return task;
    } 
 
    mEmptyCondition.Trigger(); 
    return JobProcessor::Task_t(); 
} 
