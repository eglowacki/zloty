#include "Metrics/Concurrency.h"
#include "HashUtilities.h"
#include "YagetVersion.h"
#include "App/AppUtilities.h"
#include "App/FileUtilities.h"
#include "Debugging/DevConfiguration.h"
#include "Fmt/ostream.h"
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
{}

yaget::metrics::Channel::Channel(const std::string& message, const char* file, uint32_t line)
    : internal::Metric(message, file, line)
{
}


yaget::metrics::Channel::~Channel()
{
    const std::size_t threadID = platform::CurrentThreadId();
    GetSaver().AddProfileStamp({ mMessage, mStart, platform::GetRealTime(yaget::time::kMicrosecondUnit), threadID, yaget::metrics::TraceRecord::Event::Complete });
}


yaget::metrics::TimeSpan::TimeSpan(std::size_t id, const std::string& message, const char* file, uint32_t line)
    : internal::Metric(message, file, line)
    , mId(id)
{
    if (mId)
    {
        const std::size_t threadID = platform::CurrentThreadId();
        GetSaver().AddProfileStamp({ mMessage, mStart, mStart, threadID, yaget::metrics::TraceRecord::Event::AsyncBegin, mId });
    }
}


yaget::metrics::TimeSpan::~TimeSpan()
{
    if (mId)
    {
        const std::size_t threadID = platform::CurrentThreadId();
        const auto currentTime = platform::GetRealTime(yaget::time::kMicrosecondUnit);
        GetSaver().AddProfileStamp({ mMessage, currentTime, currentTime, threadID, yaget::metrics::TraceRecord::Event::AsyncEnd, mId });
    }
}


void yaget::metrics::TimeSpan::AddMessage(const char* message) const
{
    if (mId)
    {
        const auto currentTime = platform::GetRealTime(yaget::time::kMicrosecondUnit);
        const std::size_t threadID = platform::CurrentThreadId();
        GetSaver().AddProfileStamp({ message, currentTime, currentTime, threadID, yaget::metrics::TraceRecord::Event::Async, mId });
    }
}

void yaget::metrics::MarkStartThread(uint32_t threadId, const char* threadName)
{
    //tmThreadName(CaptureMask, threadId, threadName);
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
