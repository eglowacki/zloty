///////////////////////////////////////////////////////////////////////
// JobPool.h
//
//  Copyright 07/19/2016 Edgar Glowacki.
//
//  Maintained by: Edgar
//
//  NOTES:
//     
//
//
//  #include "ThreadModel/JobPool.h"
//
//////////////////////////////////////////////////////////////////////
//! \file

#pragma once

#include "JobProcessor.h"
#include "ThreadModel/Condition.h" 
#include <functional>
#include <deque>
#include <map>

namespace yaget
{
    namespace mt
    {
        /**
        Create number of threads and put them into waiting state. By calling AddTask, threads will start
        processing them asap. Delete this object to stop all threads, 
        which will may not finish all tasks left in queue. 
        If numThreads is 0, then allocate number_of_hardware_threads - 1.
        If numThreads > 0, then allocate that many threads.
        There is no validation on upper limit and undefined behavior may occur at large values.
        What are a large values you may ask. Circa 2019 it may be thousands or so, later who knows???
        */
        class JobPool : public Noncopyable<JobPool>
        {
        public:
            using Threads_t = std::map<std::string, JobProcessor::Holder>;

            enum class Behaviour { StartAsRun, StartAsPause };
            JobPool(const char* poolName, uint32_t numThreads = 0, Behaviour behaviour = Behaviour::StartAsRun);
            ~JobPool();

            void AddTask(JobProcessor::Task_t task);
            void UnpauseAll();
            
            void Clear();

            // Blocking call, it will process all tasks until none. 
            // It is possible to add new task while Join() from other threads. 
            void Join();

            // if there is allot of task to be added, it's more efficient
            // to use this class, since it will lock mutex once,
            class Locker
            {
            public:
                Locker(JobPool& pool)
                    : mPool(pool), mMutexLock(pool.mPendingTasksMutex)
                {}

                void AddTask(JobProcessor::Task_t task)
                {
                    mPool.mTasks.push_back(task);
                }

                template <typename T>
                void AddTasks(const T& tasks)
                {
                    std::copy(std::begin(tasks), std::end(tasks),
                        std::inserter(mPool.mTasks, std::end(mPool.mTasks)));
                }

            private:
                JobPool& mPool;
                std::unique_lock<std::mutex> mMutexLock;
            };

            Locker GetLocker()
            {
                return Locker(*this);
            }

        private:
            JobProcessor::Task_t PopNextTask();

            Threads_t mThreads;
            typedef std::deque<JobProcessor::Task_t> Tasks_t;
            Tasks_t mTasks;
            std::condition_variable mTaskWaitCondition;
            std::mutex mTaskWaitMutex;
            std::mutex mPendingTasksMutex;
            std::string mName;
            Behaviour mBehaviour = Behaviour::StartAsRun;
            mt::Condition mEmptyCondition;
        };
    } // namespace mt
} // namespace yaget