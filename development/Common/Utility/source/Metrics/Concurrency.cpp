#include "Metrics/Concurrency.h"
#include "Debugging/DevConfiguration.h"
#include "Metrics/PerformanceTracer.h"
#include "Platform/Support.h"

#include <filesystem>
namespace fs = std::filesystem;

#if YAGET_CONC_METRICS_ENABLED == 1
#include <atlbase.h>

YAGET_COMPILE_GLOBAL_SETTINGS("(WIP) Concurenty Metrics Included")

namespace
{
    yaget::metrics::TraceCollector& GetSaver()
    {
        static yaget::metrics::TraceCollector profSaver;

        return profSaver;
    }
}


yaget::metrics::internal::Metric::Metric(const std::string& message, const char* file, uint32_t line)
    : mMessage(message)
    , mFileName(file ? file : "Unknown")
    , mLineNumber(line)
    , mStart(platform::GetRealTime(yaget::time::kMicrosecondUnit))
    , mTreadID(platform::CurrentThreadId())
{}


yaget::metrics::Channel::Channel(const std::string& message, const char* file, uint32_t line)
    : internal::Metric(message, file, line)
{
    GetSaver().AddProfileStamp({ mMessage, mStart, mStart, mTreadID, TraceRecord::Event::Begin, 0, "Channel" });
}


yaget::metrics::Channel::~Channel()
{
    if (mMessage == "T.TraceDataSaver.1")
    {
        int z = 0;
        z;
    }
    YAGET_ASSERT(mTreadID == platform::CurrentThreadId());

    const auto currentTime = platform::GetRealTime(yaget::time::kMicrosecondUnit);
    GetSaver().AddProfileStamp({ mMessage, currentTime, currentTime, mTreadID, TraceRecord::Event::End, 0, "Channel" });
}


void yaget::metrics::Channel::AddMessage(const std::string& message, MessageScope scope) const
{
    YAGET_ASSERT(mTreadID == platform::CurrentThreadId());

    MarkAddMessage(message, scope, 0);
}


yaget::metrics::TimeSpan::TimeSpan(std::size_t id, const std::string& message, const char* file, uint32_t line)
    : internal::Metric(message, file, line)
    , mId(id)
{
    if (mId)
    {
        GetSaver().AddProfileStamp({ mMessage, mStart, mStart, mTreadID, TraceRecord::Event::FlowBegin, mId, "Tracker" });
    }
}


yaget::metrics::TimeSpan::~TimeSpan()
{
    if (mId)
    {
        const std::size_t threadID = platform::CurrentThreadId();
        const auto currentTime = platform::GetRealTime(time::kMicrosecondUnit);
        GetSaver().AddProfileStamp({ mMessage, currentTime, currentTime, threadID, TraceRecord::Event::FlowEnd, mId, "Tracker" });
    }
}


void yaget::metrics::TimeSpan::AddMessage(const std::string& message) const
{
    if (mId)
    {
        const std::size_t threadID = platform::CurrentThreadId();
        const auto currentTime = platform::GetRealTime(time::kMicrosecondUnit);
        GetSaver().AddProfileStamp({ message, currentTime, currentTime, threadID, TraceRecord::Event::FlowPoint, mId, "Tracker" });
    }
}


yaget::metrics::Lock::Lock(const std::string& message, const char* file, uint32_t line)
    : internal::Metric(message, file, line)
    , mChannel(message, file, line)
{
    GetSaver().AddProfileStamp({ "Acquiring." + mMessage, mStart, mStart, mTreadID, TraceRecord::Event::Begin, 0, "Channel" });
}


yaget::metrics::UniqueLock::UniqueLock(std::mutex& mutex, const std::string& message, const char* file, uint32_t line)
    : Lock("Mutex:" + message, file, line)
    , mlocker(mutex)
{
    const auto currentTime = platform::GetRealTime(time::kMicrosecondUnit);
    GetSaver().AddProfileStamp({ "Acquiring." + mMessage, currentTime, currentTime, mTreadID, TraceRecord::Event::End, 0, "Channel" });
}


void yaget::metrics::MarkAddMessage(const std::string& message, MessageScope scope, size_t id)
{
    const std::size_t threadID = platform::CurrentThreadId();
    const auto currentTime = platform::GetRealTime(time::kMicrosecondUnit);
    GetSaver().AddProfileStamp({ message, currentTime, currentTime, threadID, TraceRecord::Event::Instant, id, "Tracker", scope });
}

void yaget::metrics::MarkStartThread(uint32_t threadId, const char* threadName)
{
    platform::SetThreadName(threadName, threadId);
    GetSaver().SetThreadName(threadName, threadId);
}


void yaget::metrics::MarkStartThread(std::thread& t, const char* threadName)
{
    MarkStartThread(platform::GetThreadId(t), threadName);
}


std::string yaget::metrics::MarkGetThreadName(std::thread& thread)
{
    return MarkGetThreadName(platform::GetThreadId(thread));
}


std::string yaget::metrics::MarkGetThreadName(uint32_t threadId)
{
    return platform::GetThreadName(threadId);
}


#else // YAGET_CONC_METRICS_ENABLED

YAGET_COMPILE_GLOBAL_SETTINGS("Concurenty Metrics Partialy NOT Included")
//#pragma message("======== Concurenty Metrics Partialy NOT Included ========")

void yaget::metrics::MarkStartThread(uint32_t threadId, const char* threadName)
{
    //tmThreadName(CaptureMask, threadId, threadName);
    platform::SetThreadName(threadName, threadId);
}

void yaget::metrics::MarkStartThread(std::thread& t, const char* threadName)
{
    MarkStartThread(platform::GetThreadId(t), threadName);
}

std::string yaget::metrics::MarkGetThreadName(std::thread& thread)
{
    return MarkGetThreadName(platform::GetThreadId(thread));
}

std::string yaget::metrics::MarkGetThreadName(uint32_t threadId)
{
    return platform::GetThreadName(threadId);
}

#endif // YAGET_CONC_METRICS_ENABLED
