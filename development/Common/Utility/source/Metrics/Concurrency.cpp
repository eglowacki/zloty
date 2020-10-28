#include "Metrics/Concurrency.h"
#include "App/AppUtilities.h"
#include "Fmt/ostream.h"
#include "Platform/Support.h"
#include "YagetVersion.h"
#include "Debugging/DevConfiguration.h"

#if YAGET_CONC_METRICS_ENABLED == 1
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
#else

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
