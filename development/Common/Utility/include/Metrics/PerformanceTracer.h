/////////////////////////////////////////////////////////////////////////
// PerformanceTracer.h
//
//  Copyright 2021 Edgar Glowacki.
//
// NOTES:
//      Collects chrome://tracing samples
//      c:\Users\edgar\AppData\Local\Temp\Beyond Limits\YagetCore-Test
//
// #include "Metrics/PerformanceTracer.h"
//
/////////////////////////////////////////////////////////////////////////
//! \file

#pragma once


#include "YagetCore.h"
#include "Time/GameClock.h"
#include "ThreadModel/JobPool.h"
#include <fstream>
#include <unordered_map>

#include "Concurrency.h"

namespace yaget::metrics
{
    struct TraceRecord
    {
        enum class Event { Begin, End, Complete, Instant, AsyncBegin, AsyncEnd, AsyncPoint, Lock, FlowBegin, FlowEnd, FlowPoint };

        std::string mName;
        yaget::time::TimeUnits_t mStart = 0;
        yaget::time::TimeUnits_t mEnd = 0;
        std::size_t mThreadID = 0;
        Event mEvent = Event::Complete;
        std::size_t mId = 0;
        std::string mCategory;
        MessageScope mMessageScope = MessageScope::Thread;
    };

    using ThreadNames = std::map<std::size_t, std::string>;

    class TraceCollector
    {
    public:
        TraceCollector();
        ~TraceCollector();

        void AddProfileStamp(TraceRecord&& result);

    private:
        void DataSaver();

        void SaveCurrentProfileStamps();

        std::mutex mmProfileStampMutex;
        std::mutex mmThreadNameMutex;
        using ProfileStamps = std::vector<TraceRecord>;
        ProfileStamps mProfileStamps;
        const std::string mFilePathName;

        ThreadNames mThreadNames;

        // 
        enum class TraceState {Off, StartSaver, On};
        TraceState mTraceState = TraceState::Off;

        mt::JobPool mDataSaver = mt::JobPool("TraceDataSaver", 1);
        std::atomic_bool mQuit{ false };
        mt::Condition mTracingCondition;
        std::ofstream mOutputStream;
    };

}
