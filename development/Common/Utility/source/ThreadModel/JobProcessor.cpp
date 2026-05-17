#include "ThreadModel/JobProcessor.h" 
#include "Platform/Support.h" 
#include "StringHelpers.h" 

#include "Core/ErrorHandlers.h"

#include "Debugging/Assert.h" 
#include "Logger/YLog.h" 
#include "Metrics/Concurrency.h"
#include "Metrics/Gather.h"
 
 
yaget::mt::JobProcessor::Holder::Holder(const std::string& threadName, PopNextTask_t&& popNextTask, bool allowTaskToFinish) 
    : mJobProcessor(std::make_shared<JobProcessor>(threadName, std::move(popNextTask), allowTaskToFinish)) 
{ 
} 
 
 
void yaget::mt::JobProcessor::Holder::StartProcessing() 
{ 
    std::shared_ptr<JobProcessor> processor = std::atomic_load(&mJobProcessor); 
    if (processor) 
    { 
        processor->StartProcessing(); 
    } 
} 
 
 
void yaget::mt::JobProcessor::Holder::Clear() 
{ 
    std::shared_ptr<JobProcessor> processor = std::atomic_load(&mJobProcessor); 
    if (processor) 
    { 
        if (std::atomic_compare_exchange_strong(&mJobProcessor, &processor, mClearedProcessor)) 
        { 
            mClearedProcessor = processor; 
        } 
    } 
} 
 
 
yaget::mt::JobProcessor::JobProcessor(const std::string& threadName, PopNextTask_t&& popNextTask, bool allowTaskToFinish) 
    : mThreadName(threadName) 
    , mAllowTaskToFinish{ allowTaskToFinish }
{ 
    mPauseCondition.Trigger();
    mThread = std::thread(std::ref(*this), std::move(popNextTask));
    metrics::MarkStartThread(mThread, threadName.c_str());
}
 
 
yaget::mt::JobProcessor::~JobProcessor() 
{ 
    YLOG_DEBUG("MULT", "Requesting job: '%s' to stop.", mThreadName.c_str()); 
 
    mQuit = true; 
    mTaskReadyCondition.Trigger(); 
 
    if (mThread.joinable()) 
    {
        const time::TimeUnits_t maxSleepSleep = 1000000;
        const time::TimeUnits_t units = time::kMicrosecondUnit;

        platform::SleepResult sleepResult = platform::SleepResult::OK;
        if (mAllowTaskToFinish)
        {
            platform::Sleep([this]()
            {
                return mTaskInProgress == true;
            });
        }
        else
        {
            sleepResult = platform::Sleep(maxSleepSleep, units, [this]()
            {
                return mTaskInProgress == true;
            });
        }

        if (sleepResult == platform::SleepResult::OK)
        {
            mThread.join();
        }
        else
        {
            ::TerminateThread(static_cast<HANDLE>(mThread.native_handle()), 1);

            double maxSleep = time::FromTo<double>(maxSleepSleep, units, time::kSecondUnit);
            const auto message = std::format("Job '{}' killed. Task in progress exceeded time out value: '{} {}'.", mThreadName, maxSleep, metrics::UnitName(time::kSecondUnit));
            error_handlers::Throw("MULT", message);
        }
    } 
 
    metrics::MarkEndThread(mThread); 
 
    YLOG_DEBUG("MULT", "Job: '%s' Stopped.", mThreadName.c_str()); 
} 
 
 
void yaget::mt::JobProcessor::operator()(PopNextTask_t popNextTask) 
{ 
    mPauseCondition.Wait();

    metrics::Channel channel("T." + mThreadName); 

    struct Releaser
    {
        Releaser(std::atomic_bool& value)
            : mValue(value)
        {}

        ~Releaser()
        {
            mValue = false;
        }

        std::atomic_bool& mValue;
    };

    Releaser releaser(mTaskInProgress);

    while (true) 
    { 
        if (mQuit)
        {
            break;
        }

        while (Task_t nextTask = popNextTask())
        { 
            try 
            { 
                metrics::Channel lifetimeChannel("Task Lifetime"); 
 
                mTaskInProgress = true; 
                nextTask(); 
                mTaskInProgress = false; 
            } 
            catch (const std::exception& e) 
            { 
                mTaskInProgress = false; 
                YLOG_ERROR("MULT", "Task '%s' running on '%s' thread failed with exception: '%s'.", "some_task_description", mThreadName.c_str(), e.what());
            }

            // allows us to break out even  if there are more tasks to left
            if (mQuit)
            {
                return;
            }
        }

        // we always setup task in progress in ctor to true, so the thread has a chance to
        // grab on a first iteration. There was race condition in JobPool when creating
        // dynamic threads.
        mTaskInProgress = false;
        mTaskReadyCondition.Wait();
    }
} 
 
 
void yaget::mt::JobProcessor::StartProcessing() 
{ 
    YAGET_ASSERT(!mQuit, "StartProcessing called while the thread '%s' is in exit status", mThreadName.c_str()); 
    mTaskReadyCondition.Trigger(); 
} 
