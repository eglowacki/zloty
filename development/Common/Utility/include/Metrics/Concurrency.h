///////////////////////////////////////////////////////////////////////
// Concurrency.h
//
//  Copyright 4/12/2018 Edgar Glowacki.
//
//  Maintained by: Edgar
//
//  NOTES:
//      Wrapper for Intel concurrency sdk visualizer
//      #define YAGET_CONC_METRICS_ENABLED 1
//
//      https://docs.microsoft.com/en-us/archive/blogs/visualizeparallel/concurrency-visualizer-sdk-advanced-visualization-techniques
//      https://github.com/MicrosoftDocs/visualstudio-docs/blob/master/docs/profiling/concurrency-visualizer.md
//      https://github.com/MicrosoftDocs/visualstudio-docs/blob/master/docs/debugger/allocation-hook-functions.md
//
//
//  #include "Metrics/Concurrency.h"
//
//////////////////////////////////////////////////////////////////////
//! \file
#pragma once

#include "YagetCore.h"
#include "Time/GameClock.h"
#include <memory>
#include <functional>


namespace yaget::metrics
{
#if YAGET_CONC_METRICS_ENABLED == 1

    namespace internal
    {
        class Metric : public yaget::Noncopyable<Metric>
        {
        public:
            virtual ~Metric() = default;

        protected:
            Metric(const std::string& message, const char* file, uint32_t line);

            std::string mMessage;
            const char* mFileName = nullptr;
            uint32_t mLineNumber = 0;
            time::TimeUnits_t mStart = 0;
        };
    }

    class Channel : public internal::Metric
    {
    public:

        Channel(const std::string& message, const char* file, uint32_t line);
        ~Channel() override;
    };

    //--------------------------------------------------------------------------------------------------------------
    // Marks start and end time (with span) regardless of which thread started and which one ended
    class TimeSpan : public internal::Metric
    {
    public:
        TimeSpan(std::size_t id, const std::string& message, const char* file, uint32_t line);
        ~TimeSpan() override;

        void AddMessage(const char* message) const;

    private:
        std::size_t mId = 0;
    };

    //--------------------------------------------------------------------------------------------------------------
    class Locker
    {
    public:
        Locker(std::mutex&, const char*, const char*, uint32_t) {}
    };

    //--------------------------------------------------------------------------------------------------------------
    class LockerSpan : public Locker
    {
    public:
        LockerSpan(std::mutex& mutex, const char* message, const char* file, uint32_t line)
            : Locker(mutex, message, file, line)
        {}
    };
    //--------------------------------------------------------------------------------------------------------------
    inline void Initialize(const args::Options&) {}

    // putting back intel concurrency functionality
    void MarkStartThread(std::thread& thread, const char* name);
    void MarkStartThread(uint32_t threadId, const char* name);

    std::string MarkGetThreadName(std::thread& thread);
    std::string MarkGetThreadName(uint32_t threadId);


    // not converted yet
    inline void MarkEndThread(std::thread&) {}
    inline void MarkEndThread(uint32_t) {}

    inline void MarkStartTimeSpan(uint64_t, const char*, const char*, uint32_t) {}
    inline void MarkEndTimeSpan(uint64_t, const char*, uint32_t) {}

    inline void Tick() {}

#else // YAGET_CONC_METRICS_ENABLED

    class Channel
    {
    public:
        Channel(const char*, const char*, uint32_t) {}
    };

    //--------------------------------------------------------------------------------------------------------------
    // Marks start and end time (with span) regardless of which thread started and which one ended
    class TimeSpan
    {
    public:
        TimeSpan(const char*, const char*, uint32_t) {}
    };

    //--------------------------------------------------------------------------------------------------------------
    class Locker
    {
    public:
        Locker(std::mutex&, const char*, const char*, uint32_t) {}
    };

    //--------------------------------------------------------------------------------------------------------------
    class LockerSpan : public Locker
    {
    public:
        LockerSpan(std::mutex& mutex, const char* message, const char* file, uint32_t line)
            : Locker(mutex, message, file, line)
        {}
    };
    //--------------------------------------------------------------------------------------------------------------
    inline void Initialize(const args::Options&) {}

    // putting back intel concurrency functionality
    void MarkStartThread(std::thread& thread, const char* name);
    void MarkStartThread(uint32_t threadId, const char* name);

    std::string MarkGetThreadName(std::thread& thread);
    std::string MarkGetThreadName(uint32_t threadId);


    // not converted yet
    inline void MarkEndThread(std::thread&) {}
    inline void MarkEndThread(uint32_t) {}

    inline void MarkStartTimeSpan(uint64_t, const char*, const char*, uint32_t) {}
    inline void MarkEndTimeSpan(uint64_t, const char*, uint32_t) {}

    inline void Tick() {}

#endif // YAGET_CONC_METRICS_ENABLED

} // namespace yaget::metrics


#define YAGET_METRICS_CHANNEL_FILE_LINE __FILE__, __LINE__
