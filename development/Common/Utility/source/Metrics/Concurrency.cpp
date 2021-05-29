#include "Metrics/Concurrency.h"
#include "App/AppUtilities.h"
#include "Fmt/ostream.h"
#include "Platform/Support.h"
#include "YagetVersion.h"
#include "Debugging/DevConfiguration.h"

#include <filesystem>

#include <boost/hana/functional/id.hpp>


#include "HashUtilities.h"

#include "App/FileUtilities.h"
namespace fs = std::filesystem;

#if YAGET_CONC_METRICS_ENABLED == 1
#if 0
    YAGET_COMPILE_GLOBAL_SETTINGS("Concurenty Metrics Included")

#include "rad_tm.h"

#include <cvmarkersobj.h>
namespace conc = Concurrency::diagnostic;

namespace
{
#if YAGET_CONC_METRICS_ENABLED == 1
    bool bMetricActive = true;
#else
    bool bMetricActive = false;
#endif // YAGET_CONC_METRICS_ENABLED

    uint64_t CaptureMask = 0;

    const size_t kMaxChannelName = 23;
    std::string ResolveChannelName(const char* message)
    {
        std::string resolvedName = fmt::format("{}", message ? message : "channel");
        return resolvedName.length() < kMaxChannelName ? resolvedName : std::string(resolvedName.begin(), resolvedName.begin() + kMaxChannelName);
    }

    struct TelemetryScoper
    {
        TelemetryScoper(const yaget::args::Options& /*options*/)
            : mStartTime(yaget::platform::GetRealTime(yaget::time::kMicrosecondUnit))
        {
            using namespace yaget;

#ifdef YAGET_DEBUG
            tm_library_type libType = TM_DEBUG;
#else
            tm_library_type libType = TM_RELEASE;
#endif // YAGET_DEBUG

            const auto& configuration = yaget::dev::CurrentConfiguration();
            const bool& allowSocketConnection = configuration.mDebug.mMetrics.AllowSocketConnection;
            const bool& allowFallbackToFile = configuration.mDebug.mMetrics.AllowFallbackToFile;

            tmLoadLibrary(libType);
            tmInitialize(0, 0);

            const auto captureName = util::ExpendEnv("$(AppName)", nullptr);
            const auto version = ToString(YagetVersion);
            std::string outputName = "localhost";
            tm_error err = TMERR_UNKNOWN;

            if (allowSocketConnection)
            {
                err = tmOpen(
                    0,                      // unused
                    captureName.c_str(),    // program name, don't use slashes or weird character that will screw up a filename
                    version.c_str(),        // identifier, could be date time, or a build number ... whatever you want
                    outputName.c_str(),     // telemetry server address
                    TMCT_TCP,               // network capture
                    4719,                   // telemetry server port
                    TMOF_INIT_NETWORKING,   // flags
                    100);                   // timeout in milliseconds ... pass -1 for infinite
            }

            if (err != TM_OK && allowFallbackToFile)
            {
                outputName = util::ExpendEnv("$(LogFolder)/telemetry_capture.tmcap", nullptr);
                err = tmOpen(
                    0,                      // unused
                    captureName.c_str(),    // program name, don't use slashes or weird character that will screw up a filename
                    version.c_str(),        // identifier, could be date time, or a build number ... whatever you want
                    outputName.c_str(),     // telemetry server address
                    TMCT_FILE,              // network capture
                    4719,                   // telemetry server port
                    0,                      // flags
                    -1);                    // timeout in milliseconds ... pass -1 for infinite
            }

            if (err == TM_OK)
            {
                //tmThreadName(0, 0, "y.Main");
                metrics::MarkStartThread(0, "Yaget.Main");

                YLOG_NOTICE("METR", "Connected to the Telemetry server with '%s'.", outputName.c_str());
            }
            else if (err == TMERR_DISABLED)
            {
                YLOG_NOTICE("METR", "Telemetry is disabled via #define NTELEMETRY.");
            }
            else if (err == TMERR_UNINITIALIZED)
            {
                YLOG_WARNING("METR", "tmInitialize failed or was not called.");
            }
            else if (err == TMERR_NETWORK_NOT_INITIALIZED)
            {
                YLOG_WARNING("METR", "WSAStartup was not called before tmOpen! Call WSAStartup or pass TMOF_INIT_NETWORKING.");
            }
            else if (err == TMERR_NULL_API)
            {
                YLOG_NOTICE("METR", "There is no Telemetry API (the DLL isn't in the EXE's path).");
            }
            else if (err == TMERR_COULD_NOT_CONNECT)
            {
                YLOG_NOTICE("METR", "There is no Telemetry server running.");
            }
            else if (err == TMERR_FILE_OPEN_FAILED)
            {
                YLOG_WARNING("METR", "Could not open capture file: '$s'.", outputName.c_str());
            }
        }

        ~TelemetryScoper()
        {
            auto endTime = yaget::platform::GetRealTime(yaget::time::kMicrosecondUnit);
            auto diffTime = endTime - mStartTime;
            diffTime;

            tmClose(0);
            tmShutdown();
        }

        yaget::time::Microsecond_t mStartTime = 0;
    };

    std::unique_ptr<TelemetryScoper> telemetryScoper;

} // namespace


yaget::metrics::Channel::Channel(const char* message, const char* file, uint32_t line)
{
    tmEnterEx(CaptureMask, nullptr, 0, 0, file, line, 0, message);
}


yaget::metrics::Channel::~Channel()
{
    tmLeave(CaptureMask);
}


void yaget::metrics::Initialize(const args::Options& options)
{
    telemetryScoper = nullptr;
    telemetryScoper = std::make_unique<TelemetryScoper>(options);
}


void yaget::metrics::MarkStartThread(uint32_t threadId, const char* threadName)
{
    tmThreadName(CaptureMask, threadId, threadName);
    platform::SetThreadName(threadName, threadId);
}


void yaget::metrics::MarkEndThread(uint32_t threadId)
{
    tmEndThread(CaptureMask, threadId);
}


void yaget::metrics::MarkStartThread(std::thread& t, const char* threadName)
{
    MarkStartThread(platform::GetThreadId(t), threadName);
}


void yaget::metrics::MarkEndThread(std::thread& t)
{
    MarkEndThread(platform::GetThreadId(t));
}


yaget::metrics::TimeSpan::TimeSpan(const char* message, const char* file, uint32_t line)
    : mZoneId(tmNewTimeSpanTrackID())
{
    tmBeginTimeSpanEx(CaptureMask, mZoneId, 0, file, line, message);
}


yaget::metrics::TimeSpan::~TimeSpan()
{
    tmEndTimeSpan(CaptureMask, mZoneId);
}


yaget::metrics::internal::PerfLocker::PerfLocker(Mutext& mutex, const char* message, const char* file, uint32_t line)
    : mMutex(mutex)
{
    tmStartWaitForLockEx(CaptureMask, 0, &mMutex, file, line, message);
}


yaget::metrics::internal::PerfLocker::~PerfLocker()
{
    tmReleasedLock(CaptureMask, &mMutex);
}


yaget::metrics::Locker::Locker(Mutext& mutex, const char* message, const char* file, uint32_t line)
    : PerfLocker(mutex, message, file, line)
    , mLock(mutex)
{
    tmEndWaitForLock(CaptureMask);
    tmAcquiredLock(CaptureMask, 0, &mMutex, message);
}


//yaget::metrics::LockerMarker::LockerMarker(void* mutex, const char* message, LockOperation lockOperation, const char* file, uint32_t line)
//    : mMutex(mutex)
//{
//    tmStartWaitForLockEx(0, 0, mMutex, file, line, message);
//
//    lockOperation();
//
//    tmEndWaitForLock(0);
//    tmAcquiredLock(0, 0, mMutex, message);
//}
//
//
//yaget::metrics::LockerMarker::~LockerMarker()
//{
//    tmReleasedLock(0, mMutex);
//}


void yaget::metrics::Tick()
{
    tmTick(CaptureMask);
}


void yaget::metrics::MarkStartTimeSpan(uint64_t spanId, const char* message, const char* file, uint32_t line)
{
    tmBeginTimeSpanEx(CaptureMask, spanId, 0, file, line, message);
}


void yaget::metrics::MarkEndTimeSpan(uint64_t spanId, const char* file, uint32_t line)
{
    tmEndTimeSpanEx(CaptureMask, spanId, file, line);
}
#endif // if 0

//#include "cvmarkersobj.h"
//using namespace Concurrency::diagnostic;

#include <atlbase.h>

YAGET_COMPILE_GLOBAL_SETTINGS("(WIP) Concurenty Metrics Included")


//_Check_return_ HRESULT CvInitProvider(
//    _In_ const GUID* pGuid, "8d4925ab-505a-483b-a7e0-6f824a07a6f0"
//    _Out_ PCV_PROVIDER* ppProvider);

namespace
{
    //GUID GuidFromString(const char* text)
    //{
    //    GUID guid;
    //    ::GUIDFromString(text, &guid);

    //    return guid;
    //}
    //
    //GUID StringToGuid(const std::string& str)
    //{
    //    static GUID guid;
    //    sscanf(str.c_str(),
    //        "{%8x-%4hx-%4hx-%2hhx%2hhx-%2hhx%2hhx%2hhx%2hhx%2hhx%2hhx}",
    //        &guid.Data1, &guid.Data2, &guid.Data3,
    //        &guid.Data4[0], &guid.Data4[1], &guid.Data4[2], &guid.Data4[3],
    //        &guid.Data4[4], &guid.Data4[5], &guid.Data4[6], &guid.Data4[7]);

    //    return guid;
    //}
    //

    //yaget::Guid providerGui("8d4925ab-505a-483b-a7e0-6f824a07a6f0");
    //PCV_PROVIDER provider = nullptr;

    //const GUID* GetProvider()
    //{
    //    //_Check_return_ HRESULT CvInitProvider(
    //    //    _In_ const GUID* pGuid, "8d4925ab-505a-483b-a7e0-6f824a07a6f0"
    //    //    _Out_ PCV_PROVIDER* ppProvider);

    //    const GUID* guid = reinterpret_cast<const GUID*>(&providerGui.bytes()[0]);

    //    HRESULT result = CvInitProvider(guid, &provider);
    //    result;
    //    int z = 0;
    //    z;

    //    return guid;
    //}

    struct ProfileResult
    {
        enum class Event { Complete, Instant, AsyncBegin, AsyncEnd, Async };

        std::string mName;
        yaget::time::TimeUnits_t mStart = 0;
        yaget::time::TimeUnits_t mEnd = 0;
        std::size_t mThreadID = 0;
        Event mEvent = Event::Complete;
        std::size_t mId = 0;
    };

    struct ProfSaver
    {
        ProfSaver()
            : mFilePathName(fs::path(yaget::util::ExpendEnv("$(Temp)/results.json", nullptr)).generic_string())
        {
            using namespace yaget;

            if (io::file::IsFileExists(mFilePathName))
            {
                int lastFileIndex = 0;
                const auto logNames = io::file::GetFileNames(fs::path(yaget::util::ExpendEnv("$(Temp)", nullptr)).generic_string(), false, "*results-????.json");
                for (const auto& name : logNames)
                {
                    std::string_view v{ name };
                    v.remove_prefix(v.size() - 9);
                    v.remove_suffix(5);

                    std::string value(v.begin(), v.end());
                    int fileIndex = conv::AtoN<int>(value.c_str());
                    lastFileIndex = std::max(lastFileIndex, fileIndex);
                }

                const std::string postfix = fmt::format("-{:04}.json", lastFileIndex + 1);
                const std::string newName = fs::path(yaget::util::ExpendEnv("$(Temp)/results" + postfix, nullptr)).generic_string();

                const auto& [result, errorMessage] = io::file::RenameFile(mFilePathName, newName);
            }
        }

        ~ProfSaver()
        {
            ProfileStamps profileStamps;
            {
                std::unique_lock<std::mutex> mutexLock(mmProfileStampMutex);
                std::swap(profileStamps, mProfileStamps);
            }

            if (!profileStamps.empty())
            {
                std::ofstream outputStream(mFilePathName.c_str());

                outputStream << "{\"otherData\": {";
                    outputStream << "\"Application\": \"Yaget-Test-Core\",";
                    outputStream << "\"Date\": \"Saturday May 29, 2021. 12:59PM\"";
                    outputStream << "},";

                outputStream << "\"traceEvents\":[";

                int profileCount = 0;
                for (const auto& profileStamp : profileStamps)
                {
                    std::string name = profileStamp.mName;
                    std::replace(name.begin(), name.end(), '"', '\'');

                    switch (profileStamp.mEvent)
                    {
                    case ProfileResult::Event::Complete:
                        if (profileCount++ > 0)
                        {
                            outputStream << ",";
                        }
                        outputStream << "{";
                        outputStream << "\"cat\": \"function\",";
                        outputStream << "\"dur\": " << (profileStamp.mEnd - profileStamp.mStart) << ',';
                        outputStream << "\"name\": \"" << name << "\",";
                        outputStream << "\"ph\": \"X\",";
                        outputStream << "\"pid\": 0,";
                        outputStream << "\"tid\": " << profileStamp.mThreadID << ",";
                        outputStream << "\"ts\": " << profileStamp.mStart;
                        outputStream << "}";

                        break;
                    case ProfileResult::Event::AsyncBegin:
                        if (profileCount++ > 0)
                        {
                            outputStream << ",";
                        }
                        outputStream << "{";
                        outputStream << "\"cat\": \"async\",";
                        outputStream << "\"name\": \"" << name << "\",";
                        outputStream << "\"id\": " << profileStamp.mId << ",";
                        outputStream << "\"ph\": \"b\",";
                        outputStream << "\"pid\": 0,";
                        outputStream << "\"tid\": " << profileStamp.mThreadID << ",";
                        outputStream << "\"ts\": " << profileStamp.mStart << ",";
                        outputStream << "\"args\": {";
                        outputStream << "\"name\": " << "\"~/.bashrc\"" << "}";
                        outputStream << "}";

                        break;
                    case ProfileResult::Event::AsyncEnd:
                        if (profileCount++ > 0)
                        {
                            outputStream << ",";
                        }
                        outputStream << "{";
                        outputStream << "\"cat\": \"async\",";
                        outputStream << "\"name\": \"" << name << "\",";
                        outputStream << "\"tid\": " << profileStamp.mThreadID << ",";
                        outputStream << "\"id\": " << profileStamp.mId << ",";
                        outputStream << "\"pid\": 0,";
                        outputStream << "\"ts\": " << profileStamp.mEnd << ",";
                        outputStream << "\"ph\": \"e\"";
                        outputStream << "}";

                        break;
                    case ProfileResult::Event::Async:
                        if (profileCount++ > 0)
                        {
                            outputStream << ",";
                        }
                        outputStream << "{";
                        outputStream << "\"cat\": \"async\",";
                        outputStream << "\"name\": \"" << name << "\",";
                        outputStream << "\"tid\": " << profileStamp.mThreadID << ",";
                        outputStream << "\"id\": " << profileStamp.mId << ",";
                        outputStream << "\"pid\": 0,";
                        outputStream << "\"ts\": " << profileStamp.mStart << ",";
                        outputStream << "\"ph\": \"n\"";
                        outputStream << "}";

                        break;
                    }

                    //if (profileCount == 500)
                    //{
                    //    outputStream << ",";
                    //    outputStream << "{";
                    //    outputStream << "\"name\": \"500 Ping\",";
                    //    outputStream << "\"ph\": \"i\",";
                    //    outputStream << "\"pid\": 0,";
                    //    outputStream << "\"tid\": " << profileStamp.ThreadID << ",";
                    //    outputStream << "\"ts\": " << profileStamp.Start << ",";
                    //    outputStream << "\"s\": \"t\"";
                    //    outputStream << "}";
                    //}
                }

                for (const auto& [id, name] : mThreadNames)
                {
                    if (profileCount++ > 0)
                    {
                        outputStream << ",";
                    }

                    outputStream << "{";
                    outputStream << "\"name\": \"thread_name\",";
                    outputStream << "\"ph\": \"M\",";
                    outputStream << "\"pid\": 0,";
                    outputStream << "\"tid\": " << id << ",";
                    outputStream << "\"args\": {";
                    outputStream << "\"name\": \"" << name << "\"}";
                    outputStream << "}";
                }

                outputStream << "]}";
                outputStream.flush();
            }
        }

        void AddProfileStamp(ProfileResult&& result)
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

        void SetThreadName(const char* threadName, std::size_t t)
        {
            std::unique_lock<std::mutex> mutexLock(mmProfileStampMutex);
            mThreadNames[t] = threadName ? threadName : "";
        }

        std::mutex mmProfileStampMutex;
        using ProfileStamps = std::vector<ProfileResult>;
        ProfileStamps mProfileStamps;
        const std::string mFilePathName;

        using ThreadNames = std::unordered_map<std::size_t, std::string>;
        ThreadNames mThreadNames;
    };

    ProfSaver& GetSaver()
    {
        static ProfSaver profSaver;

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
    GetSaver().AddProfileStamp({ mMessage, mStart, platform::GetRealTime(yaget::time::kMicrosecondUnit), threadID, ProfileResult::Event::Complete });
}


yaget::metrics::TimeSpan::TimeSpan(std::size_t id, const char* message, const char* file, uint32_t line)
    : internal::Metric(message, file, line)
    , mId(id)
{
    if (mId)
    {
        const std::size_t threadID = platform::CurrentThreadId();
        GetSaver().AddProfileStamp({ mMessage, mStart, platform::GetRealTime(yaget::time::kMicrosecondUnit), threadID, ProfileResult::Event::AsyncBegin, mId });
    }
}


yaget::metrics::TimeSpan::~TimeSpan()
{
    if (mId)
    {
        const std::size_t threadID = platform::CurrentThreadId();
        GetSaver().AddProfileStamp({ mMessage, mStart, platform::GetRealTime(yaget::time::kMicrosecondUnit), threadID, ProfileResult::Event::AsyncEnd, mId });
    }
}


void yaget::metrics::TimeSpan::AddMessage(const char* message) const
{
    if (mId)
    {
        const auto currentTime = platform::GetRealTime(yaget::time::kMicrosecondUnit);
        const std::size_t threadID = platform::CurrentThreadId();
        GetSaver().AddProfileStamp({ message, currentTime, currentTime, threadID, ProfileResult::Event::Async, mId });
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
