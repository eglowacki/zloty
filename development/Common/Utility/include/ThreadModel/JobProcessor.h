/////////////////////////////////////////////////////////////////////// 
// JobProcessor.h 
// 
//  Copyright 07/19/2016 Edgar Glowacki. 
// 
//  Maintained by: Edgar 
// 
//  NOTES: 
//      
// 
// 
//  #include "ThreadModel/JobProcessor.h" 
// 
////////////////////////////////////////////////////////////////////// 
//! \file 
#pragma once 
 
#include "ThreadModel/Condition.h" 
#include <functional> 
#include <memory> 
#include <atomic> 
 
namespace yaget 
{ 
    namespace mt 
    { 
        // Creates number of threads and feeds with tasks by using provided callback PopNextTask_t 
        // This class is used by JobPool 
        class JobProcessor : public Noncopyable<JobProcessor> 
        { 
        public: 
            using Task_t = std::function<void()>; 
            using PopNextTask_t = std::function<Task_t()>; 
 
            struct Holder 
            { 
                Holder(const std::string& threadName, PopNextTask_t&& popNextTask); 
                void StartProcessing(); 
 
                void Clear();
                // this thread is processing some task
                [[nodiscard]] bool IsBusy() const { return mJobProcessor->mTaskInProgress == true; }
 
            private: 
                std::shared_ptr<JobProcessor> mJobProcessor; 
                std::shared_ptr<JobProcessor> mClearedProcessor; 
            }; 
 
            JobProcessor(const std::string& threadName, PopNextTask_t&& popNextTask); 
            ~JobProcessor(); 
            void operator()(PopNextTask_t popNextTask); 
 
            void StartProcessing(); 
 
        private: 
            bool mQuit = false; 
            const std::string mThreadName; 
 
            std::thread mThread; 
            std::atomic_bool mTaskInProgress{ true }; 
 
            Condition mTaskReadyCondition; 
            Condition mPauseCondition; 
        }; 
    } // namespace mt 
} // namespace yaget 
