//#define YAGET_GET_STRUCT_SIZE
#include "Metrics/PerformanceTracer.h"
#include "App/AppUtilities.h"
#include "App/FileUtilities.h"
#include "StringHelpers.h"
#include "Json/JsonHelpers.h"

#include <filesystem>
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
        "i",    // Instant
        "b",    // AsyncBegin
        "e",    // AsyncEnd
        "n",    // AsyncPoint
        "X",    // Lock
        "s",    // FlowBegin
        "f",    // FlowEnd
        "t"     // FlowPoint
    };
}

namespace yaget::metrics
{
    inline void to_json(nlohmann::json& j, const yaget::metrics::TraceRecord& profileStamp)
    {
        j["name"] = profileStamp.mName;
        j["pid"] = 0;
        j["tid"] = profileStamp.mThreadID;
        j["ts"] = profileStamp.mStart;
        j["ph"] = PH[static_cast<int>(profileStamp.mEvent)];
        j["cat"] = profileStamp.mCategory;

        switch (profileStamp.mEvent)
        {
        case yaget::metrics::TraceRecord::Event::Complete:
            j["dur"] = profileStamp.mEnd - profileStamp.mStart;

            break;
        case yaget::metrics::TraceRecord::Event::Begin:
        case yaget::metrics::TraceRecord::Event::End:
        case yaget::metrics::TraceRecord::Event::AsyncEnd:
        case yaget::metrics::TraceRecord::Event::FlowEnd:
        case yaget::metrics::TraceRecord::Event::AsyncPoint:
        case yaget::metrics::TraceRecord::Event::FlowPoint:
        case yaget::metrics::TraceRecord::Event::AsyncBegin:
        case yaget::metrics::TraceRecord::Event::FlowBegin:
            j["id"] = profileStamp.mId;

            break;
        case yaget::metrics::TraceRecord::Event::Instant:

            break;
        case yaget::metrics::TraceRecord::Event::Lock:
            j["dur"] = profileStamp.mEnd - profileStamp.mStart;
            //j["cname"] = "terrible";

            break;
        }
    }

    inline void to_json(nlohmann::json& j, const yaget::metrics::ThreadNames::value_type& threadInfo)
    {
        j["name"] = "thread_name";
        j["ph"] = "M";
        j["pid"] = 0;
        j["tid"] = threadInfo.first;
        j["args"]["name"] = threadInfo.second;
    }

    inline void to_json(nlohmann::json& j, const yaget::metrics::ThreadNames& threadNames)
    {
        for (const auto& it : threadNames)
        {
            nlohmann::json j2;
            to_json(j2, it);
            j.push_back(j2);
        }
    }
}


yaget::metrics::TraceCollector::TraceCollector()
    : mFilePathName(fs::path(yaget::util::ExpendEnv("$(Temp)/$(AppName)_trace.json", nullptr)).generic_string()) // todo: trace file name should be driven by config
{
    using namespace yaget;

    yaget::util::FileCycler("$(Temp)", "$(AppName)_trace", "json");
}


yaget::metrics::TraceCollector::~TraceCollector()
{
    ProfileStamps profileStamps;
    {
        std::unique_lock<std::mutex> mutexLock(mmProfileStampMutex);
        std::swap(profileStamps, mProfileStamps);
    }

    if (!profileStamps.empty())
    {
        ////profileStamps.erase(profileStamps.begin() + 5, profileStamps.end());

        std::ofstream outputStream(mFilePathName.c_str());

        outputStream << "{\"otherData\": {";
        outputStream << "\"Application\": \"Yaget-Test-Core\",";
        outputStream << "\"Date\": \"Saturday May 29, 2021. 12:59PM\"";
        outputStream << "},";

        outputStream << "\"traceEvents\":";

        nlohmann::json jsonBlock = profileStamps;
        to_json(jsonBlock, mThreadNames);

        outputStream << jsonBlock;
        outputStream << "}";
        outputStream.flush();
    }
}


void yaget::metrics::TraceCollector::AddProfileStamp(yaget::metrics::TraceRecord&& result)
{
    std::size_t num = 0;
    {
        std::unique_lock<std::mutex> mutexLock(mmProfileStampMutex);
        mProfileStamps.emplace_back(std::move(result));
        num = mProfileStamps.size();
    }

    const int capacityChecker = 1000;
    if (num % capacityChecker == 0)
    {
        const std::string bytes = yaget::conv::ToThousandsSep(num * sizeof(ProfileStamps::value_type));
        YLOG_DEBUG("METR", "Accumulated profile results: '%d' results using '%s' bytes of memory.", num, bytes.c_str());
    }
}

void yaget::metrics::TraceCollector::SetThreadName(const char* threadName, std::size_t t)
{
    std::unique_lock<std::mutex> mutexLock(mmProfileStampMutex);
    mThreadNames[t] = threadName ? threadName : "";
}
