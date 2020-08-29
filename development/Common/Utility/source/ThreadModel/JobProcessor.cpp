#include "ThreadModel/JobProcessor.h" 
#include "Platform/Support.h" 
#include "Fmt/format.h" 
#include "StringHelpers.h" 
#include "Debugging/Assert.h" 
#include "Logger/YLog.h" 
#include "Metrics/Concurrency.h" 
 
 
yaget::mt::JobProcessor::Holder::Holder(const std::string& threadName, PopNextTask_t&& popNextTask) 
    : mJobProcessor(std::make_shared<JobProcessor>(threadName, std::move(popNextTask))) 
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
 
 
yaget::mt::JobProcessor::JobProcessor(const std::string& threadName, PopNextTask_t&& popNextTask) 
    : mThreadName(threadName) 
{ 
    mThread = std::thread(std::ref(*this), std::move(popNextTask)); 
    metrics::MarkStartThread(mThread, threadName.c_str()); 
 
    mPauseCondition.Trigger(); 
} 
 
 
yaget::mt::JobProcessor::~JobProcessor() 
{ 
    YLOG_DEBUG("MULT", "Requesting job: '%s' to stop.", mThreadName.c_str()); 
 
    mQuit = true; 
    mTaskReadyCondition.Trigger(); 
 
    if (mThread.joinable()) 
    { 
        mThread.join(); 
    } 
 
    metrics::MarkEndThread(mThread); 
 
    YLOG_DEBUG("MULT", "Job: '%s' Stopped.", mThreadName.c_str()); 
} 
 
 
void yaget::mt::JobProcessor::operator()(PopNextTask_t popNextTask) 
{ 
    mPauseCondition.Wait(); 
    metrics::Channel channel("Thread Lifetime", YAGET_METRICS_CHANNEL_FILE_LINE); 
 
    while (true) 
    { 
        { 
            mTaskReadyCondition.Wait(); 
 
            if (mQuit) 
            { 
                break; 
            } 
        } 
 
        while (Task_t nextTask = popNextTask()) 
        { 
            if (mQuit) 
            { 
                break; 
            } 
 
            try 
            { 
                metrics::Channel channel("Task Lifetime", YAGET_METRICS_CHANNEL_FILE_LINE); 
 
                mTaskInProgress = true; 
                nextTask(); 
                mTaskInProgress = false; 
            } 
            catch (const std::exception& e) 
            { 
                mTaskInProgress = false; 
                YLOG_ERROR("MULT", "Task failed with exception: '%s'.", e.what()); 
            } 
        } 
    } 
} 
 
 
void yaget::mt::JobProcessor::StartProcessing() 
{ 
    YAGET_ASSERT(!mQuit, "StartProcessing called while the thread '%s' is in exit status", mThreadName.c_str()); 
    mTaskReadyCondition.Trigger(); 
} 
