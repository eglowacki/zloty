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
//
//  #include "Metrics/Concurrency.h"
//
//////////////////////////////////////////////////////////////////////
//! \file
#pragma once

#include "YagetCore.h"
#include <memory>
#include <functional>


namespace yaget
{
    namespace metrics
    {
#if YAGET_CONC_METRICS_ENABLED == 1

        //--------------------------------------------------------------------------------------------------------------
        // Marks thread span
        class Channel : public yaget::Noncopyable<Channel>
        {
        public:
            Channel(const char* message, const char* file, uint32_t line);
            ~Channel();
        };

        //--------------------------------------------------------------------------------------------------------------
        // Marks start and end time (with span) regardless of which thread started and which one ended
        class TimeSpan : public yaget::Noncopyable<TimeSpan>
        {
        public:
            TimeSpan(const char* message, const char* file, uint32_t line);
            ~TimeSpan();

        private:
            uint32_t mZoneId;
        };

        //--------------------------------------------------------------------------------------------------------------
        namespace internal
        {
            class PerfLocker : public Noncopyable<PerfLocker>
            {
            public:
                using Mutext = std::mutex;

                PerfLocker(Mutext& mutex, const char* message, const char* file, uint32_t line);
                virtual ~PerfLocker();

            protected:
                Mutext& mMutex;
            };
        } // namespace internal

        //--------------------------------------------------------------------------------------------------------------
        class Locker : public internal::PerfLocker
        {
        public:
            //using Mutext = internal::PerfLocker::Mutext;

            Locker(Mutext& mutex, const char* message, const char* file, uint32_t line);

        private:
            std::unique_lock<Mutext> mLock;
        };

        //--------------------------------------------------------------------------------------------------------------
        class LockerSpan : public Locker
        {
        public:
            LockerSpan(Mutext& mutex, const char* message, const char* file, uint32_t line)
                : Locker(mutex, message, file, line)
                , mSpan(message, file, line)
            {}

        private:
            metrics::Channel mSpan;
        };

        ////--------------------------------------------------------------------------------------------------------------
        //class LockerMarker : public Noncopyable<LockerMarker>
        //{
        //public:
        //    using LockOperation = std::function<void()>;
        //    LockerMarker(void* mutex, const char* message, LockOperation lockOperation, const char* file, uint32_t line);
        //    virtual ~LockerMarker();

        //private:
        //    void* mMutex;
        //};

        //--------------------------------------------------------------------------------------------------------------
        void Initialize(const args::Options& options);

        void MarkStartThread(std::thread& t, const char* threadName);
        void MarkEndThread(std::thread& t);
        void MarkStartThread(uint32_t threadId, const char* threadName);
        void MarkEndThread(uint32_t threadId);

        void MarkStartTimeSpan(uint64_t spanId, const char* message, const char* file, uint32_t line);
        void MarkEndTimeSpan(uint64_t spanId, const char* file, uint32_t line);

        void Tick();
#else

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

        inline void MarkStartThread(std::thread&, const char*) {}
        inline void MarkEndThread(std::thread&) {}
        inline void MarkStartThread(uint32_t, const char*) {}
        inline void MarkEndThread(uint32_t) {}

        inline void MarkStartTimeSpan(uint64_t, const char*, const char*, uint32_t) {}
        inline void MarkEndTimeSpan(uint64_t, const char*, uint32_t) {}

        inline void Tick() {}

#endif // YAGET_CONC_METRICS_ENABLED

    } // namespace metric
} // namespace yaget


#define YAGET_METRICS_CHANNEL_FILE_LINE __FILE__, __LINE__
