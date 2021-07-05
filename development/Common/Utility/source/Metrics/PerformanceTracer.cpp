//#define YAGET_GET_STRUCT_SIZE
#include "Metrics/PerformanceTracer.h"
#include "App/AppUtilities.h"
#include "App/FileUtilities.h"
#include "StringHelpers.h"
#include "Json/JsonHelpers.h"

#include <filesystem>

#include "Debugging/DevConfiguration.h"


#include "Platform/Support.h"
namespace fs = std::filesystem;

const int holder = yaget::meta::print_size_at_compile<yaget::metrics::TraceRecord>();

namespace
{
    //enum class Event { Begin, End, Complete, Instant, AsyncBegin, AsyncEnd, AsyncPoint, Lock, FlowBegin, FlowEnd, FlowPoint };
    const char* PH[]
    {
        "B",    // Begin
        "E",    // End
        "X",    // Complete
        "I",    // Instant
        "b",    // AsyncBegin
        "e",    // AsyncEnd
        "n",    // AsyncPoint
        "X",    // Lock
        "s",    // FlowBegin
        "f",    // FlowEnd
        "t"     // FlowPoint
    };

    //enum class MessageScope { Global, Process, Thread };
    const char* S[]
    {
        "g",    // Global
        "p",    // Process
        "t"     // Thread
    };


    std::string ResolveTraceFileName()
    {
        const auto& name = yaget::dev::CurrentConfiguration().mDebug.mMetrics.TraceFileName;
        const auto traceFile = fs::path(yaget::util::ExpendEnv(name, nullptr)).generic_string();
        const auto [result, error] = yaget::io::file::AssureDirectories(traceFile);

        return result ? traceFile : "";
    }

    void SaveTraceRecord(const yaget::metrics::TraceRecord& profileStamp, std::ofstream& file)
    {
        file << std::setprecision(3) << std::fixed;
        file << ",{";
        file << "\"name\":\"" << profileStamp.mName << "\",";
        file << "\"pid\":0,";
        file << "\"tid\":" << profileStamp.mThreadID << ",";
        file << "\"ph\":\"" << PH[static_cast<int>(profileStamp.mEvent)] << "\",";
        file << "\"cat\":\"" << profileStamp.mCategory << "\",";
        file << "\"ts\":" << profileStamp.mStart;

        switch (profileStamp.mEvent)
        {
        case yaget::metrics::TraceRecord::Event::Complete:
        case yaget::metrics::TraceRecord::Event::Lock:
            file << ",\"dur\":" << profileStamp.mEnd - profileStamp.mStart;

            break;
        case yaget::metrics::TraceRecord::Event::Begin:
        case yaget::metrics::TraceRecord::Event::End:
        case yaget::metrics::TraceRecord::Event::AsyncEnd:
        case yaget::metrics::TraceRecord::Event::FlowEnd:
        case yaget::metrics::TraceRecord::Event::AsyncPoint:
        case yaget::metrics::TraceRecord::Event::FlowPoint:
        case yaget::metrics::TraceRecord::Event::AsyncBegin:
        case yaget::metrics::TraceRecord::Event::FlowBegin:
            file << ",\"id\":" << profileStamp.mId;

            break;
        case yaget::metrics::TraceRecord::Event::Instant:
            file << ",\"s\":\"" << S[static_cast<int>(profileStamp.mMessageScope)] << "\"";;
            break;
        }

        file << "}";
    }
}


//-------------------------------------------------------------------------------------------------
yaget::metrics::TraceCollector::TraceCollector()
    : mFilePathName(ResolveTraceFileName())
    , mTraceState(mFilePathName.empty() ? TraceState::Off : (dev::CurrentConfiguration().mDebug.mMetrics.TraceOn && util::FileCycler(mFilePathName) ? TraceState::StartSaver : TraceState::Off))
{
    if (mTraceState == TraceState::StartSaver)
    {
        mOutputStream.open(mFilePathName.c_str());
        if (mOutputStream.is_open())
        {
            const auto appName = util::ExpendEnv("$(AppName)", nullptr);
            const auto dateString = platform::GetCurrentDateTime();

            mOutputStream << "{\"otherData\": {";
            mOutputStream << "\"Application\": \"" + appName + "\",";
            mOutputStream << "\"Date\": \"" + dateString + "\"";
            mOutputStream << "},";

            mOutputStream << "\"traceEvents\":[{}";
            mOutputStream.flush();
        }
        else
        {
            mTraceState = TraceState::Off;
        }
    }
}


//-------------------------------------------------------------------------------------------------
yaget::metrics::TraceCollector::~TraceCollector()
{
    mQuit = true;

    if (mTraceState == TraceState::On)
    {
        mTracingCondition.Trigger();
        mDataSaver.JoinDestroy();

        mTraceState = TraceState::Off;
        SaveCurrentProfileStamps();
        YAGET_ASSERT(mProfileStamps.empty(), "There are still '%d' entries left in Profile Stamps.", mProfileStamps.size());

        for (const auto& [id, name] : mThreadNames)
        {
            mOutputStream << ",{";
            mOutputStream << "\"name\":\"thread_name\",";
            mOutputStream << "\"ph\":\"M\",";
            mOutputStream << "\"pid\":0,";
            mOutputStream << "\"tid\":" << id << ",";
            mOutputStream << "\"args\":{\"name\":\"" << name << "\"}";

            mOutputStream << "}";
        }
        
        mOutputStream << "]}";
        mOutputStream.flush();
    }
}


//-------------------------------------------------------------------------------------------------
void yaget::metrics::TraceCollector::AddProfileStamp(yaget::metrics::TraceRecord&& result)
{
    if (mTraceState == TraceState::StartSaver)
    {
        std::unique_lock<std::mutex> mutexLock(mmProfileStampMutex);
        mDataSaver.AddTask([this]() { DataSaver(); });
        mTraceState = TraceState::On;
    }

    if (mTraceState == TraceState::On)
    {
        std::size_t num = 0;
        {
            std::unique_lock<std::mutex> mutexLock(mmProfileStampMutex);
            mProfileStamps.emplace_back(std::move(result));
            num = mProfileStamps.size();
        }

        const int capacityChecker = 5000;
        if (num % capacityChecker == 0)
        {
            const std::string bytes = yaget::conv::ToThousandsSep(num * sizeof(ProfileStamps::value_type));
            YLOG_DEBUG("METR", "Accumulated profile results: '%d' results using '%s' bytes of memory.", num, bytes.c_str());
        }
    }
}


//-------------------------------------------------------------------------------------------------
void yaget::metrics::TraceCollector::SetThreadName(const char* threadName, std::size_t t)
{
    std::unique_lock<std::mutex> mutexLock(mmThreadNameMutex);
    mThreadNames[t] = threadName ? threadName : "";
}


//-------------------------------------------------------------------------------------------------
void yaget::metrics::TraceCollector::DataSaver()
{
    do
    {
        mTracingCondition.Wait(200);

        SaveCurrentProfileStamps();
    }
    while (!mQuit);

    SaveCurrentProfileStamps();
}


//-------------------------------------------------------------------------------------------------
void yaget::metrics::TraceCollector::SaveCurrentProfileStamps()
{
    static size_t counter = 0;
    ProfileStamps profileStamps;
    {
        std::unique_lock<std::mutex> mutexLock(mmProfileStampMutex);
        std::swap(profileStamps, mProfileStamps);

        counter += profileStamps.size();;
    }

    //if (counter > 200)
    //{
    //    return;
    //}

    const auto startTime = platform::GetRealTime(yaget::time::kMicrosecondUnit);
    for (const auto& profileStamp : profileStamps)
    {
        SaveTraceRecord(profileStamp, mOutputStream);
    }

    const auto endTime = platform::GetRealTime(yaget::time::kMicrosecondUnit);
    TraceRecord traceRecord{ "TraceFileWrite", startTime, endTime, platform::CurrentThreadId(), TraceRecord::Event::Complete, 0, "FileWrite" };
    SaveTraceRecord(traceRecord, mOutputStream);
}
