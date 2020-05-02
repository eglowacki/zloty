///////////////////////////////////////////////////////////////////////
// JobQueue.h
//
//  Copyright 11/4/2006 Edgar Glowacki.
//
//  Maintained by: Edgar
//
//  NOTES:
//      Thread which accepts jobs and calls ProcessJob method on each one
//      within context of second thread
//
//
//  #include "Synchronization/JobQueue.h"
//
//////////////////////////////////////////////////////////////////////
//! \file


#ifndef SYNCHRONIZATION_JOB_QUEUE_H
#define SYNCHRONIZATION_JOB_QUEUE_H
#pragma once

#include "Base.h"
#if 0

//#include "AtomicValue.h"
//#include "ConditionObject.h"
#include <list>
#pragma warning(push)
#pragma warning (disable : 4512)  // '' : assignment operator could not be generated
#pragma warning (disable : 4244)  // '' : conversion from '' to '', possible loss of data
#include <boost/signal.hpp>
#pragma warning(pop)


namespace eg
{
    namespace job
    {
        template <typename T>
        class Task
        {
        public:
            Task(const std::string& name, T taskData, boost::function<void (T)> callback)
            : mTaskData(taskData)
            , mCallback(callback)
            , mName(name)
            {
            }

            void operator()() const
            {
                mCallback(mTaskData);
            }

        private:
            std::string mName;
            T mTaskData;
            boost::function<void (T)> mCallback;
        };


    } // namespace job

} // namespace eg
#endif // 0


#endif // SYNCHRONIZATION_JOB_QUEUE_H

