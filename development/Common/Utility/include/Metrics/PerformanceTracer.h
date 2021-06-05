/////////////////////////////////////////////////////////////////////////
// PerformanceTracer.h
//
//  Copyright 2021 Edgar Glowacki.
//
// NOTES:
//      Collects chrome://tracing samples
//
// #include "Metrics/PerformanceTracer.h"
//
/////////////////////////////////////////////////////////////////////////
//! \file

#pragma once


#include "YagetCore.h"
#include "Time/GameClock.h"
#include <unordered_map>


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
    };

    using ThreadNames = std::map<std::size_t, std::string>;

    class TraceCollector
    {
    public:
        TraceCollector();
        ~TraceCollector();

        void AddProfileStamp(TraceRecord&& result);
        void SetThreadName(const char* threadName, std::size_t t);

    private:
        std::mutex mmProfileStampMutex;
        using ProfileStamps = std::vector<TraceRecord>;
        ProfileStamps mProfileStamps;
        const std::string mFilePathName;

        ThreadNames mThreadNames;
        const bool mTraceOn = true;
    };

}
