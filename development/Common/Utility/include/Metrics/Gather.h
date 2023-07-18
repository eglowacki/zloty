//////////////////////////////////////////////////////////////////////
// Gather.h
//
//  Copyright 3/28/2019 Edgar Glowacki
//
//  Maintained by: Edgar
//
//  NOTES:
//      Provides gathering of various metrics 
//      and counters related to performance measurement.
//
//
//  #include "Metrics/Gather.h"
//
//////////////////////////////////////////////////////////////////////
//! \file
#pragma once

#include "Time/GameClock.h"
#include "Platform/Support.h"
#include "StringCRC.h"
#include "Debugging/Assert.h"
#include "Logger/YLog.h"
#include "StringHelpers.h"
#include <unordered_map>
#include <stack>
#include <map>


#define YAGET_METRIC_GATHER 0 //

namespace yaget
{
    namespace metrics
    {

        //------------------------------------------------------------------------------------------------------------------------------------------------------
        // class for keeping and organizing metrics.
        class Gather : public Noncopyable<Gather>
        {
        public:
            using IdMarker = uint32_t;

            struct MarkerData
            {
                std::string mName;
                const IdMarker mMarkerId;
                const IdMarker mParentMarker;
                time::Raw_t mTime = 0;
                uint64_t mHits = 0;
            };

            using MarkersTree = std::unordered_map<IdMarker, MarkerData>;

            Gather();
            ~Gather();

            void Add(IdMarker idMarker, time::Raw_t timeValue, const char* name);
            void PushParent(IdMarker idMarker);
            void PopParent(IdMarker idMarker);
            void Reset();

            MarkersTree GetResults() const;
            bool IsActive() const { return mActive; }

            static const Gather& Get();
            static void Activate(bool activate);

        private:
            MarkersTree mMarkers;
            std::stack<const MarkerData*> mContextStack;
            bool mActive = true;
        };

        namespace internal
        {
            extern Gather gatherer;
        }

        //------------------------------------------------------------------------------------------------------------------------------------------------------
        class Scoper
        {
        public:
            Scoper(Gather& gather, const char* name, const char* extraText, bool parent) 
                : mGather(gather)
                , mActive(gather.IsActive())
            {
                if (mActive)
                {
                    YAGET_ASSERT(name, "Metric Gather Scoper required to have a Name.");

                    mMarkerId = conv::crc32_helper(name, std::strlen(name), 0xFFFFFFFF);
                    mName = name;
                    mExtraText = extraText;
                    mParent = parent;
                    mStartTime = platform::GetRealTime(time::kRawUnit);

                    if (mParent)
                    {
                        mGather.PushParent(mMarkerId);
                    }
                }
            }

            ~Scoper()
            {
                if (mActive)
                {
                    const time::Raw_t result = platform::GetRealTime(time::kRawUnit) - mStartTime;

                    if (mParent)
                    {
                        mGather.PopParent(mMarkerId);
                    }

                    mGather.Add(mMarkerId, result, mName);
                }
            }

        private:
            Gather& mGather;
            bool mActive = false;
            Gather::IdMarker mMarkerId = 0;
            const char* mName = nullptr;
            const char* mExtraText = nullptr;
            bool mParent = false;
            time::Raw_t mStartTime = 0;
        };

        inline std::string UnitName(time::TimeUnits_t timeUnits)
        {
            if (timeUnits - time::kMicrosecondUnit == 0)
            {
                return "mc";
            }
            else if (timeUnits - time::kMilisecondUnit == 0)
            {
                return "ms";
            }
            else if (timeUnits - time::kSecondUnit == 0)
            {
                return "sec";
            }
            else if (timeUnits - time::kRawUnit == 0)
            {
                return "raw";
            }

            return "?";

        }

        template<int64_t TU = time::kMicrosecondUnit>
        class TimeScoper
        {
        public:
            //YAGET_LOG_FILE_LINE_FUNCTION
            TimeScoper(const char* message, const char* file, uint32_t line, const char* function)
                : TimeScoper("PROF", message, file, line, function)
            {}

            TimeScoper(const char* tag, const char* message, const char* file, uint32_t line, const char* function)
                : mStartTime(platform::GetRealTime(time::kMicrosecondUnit))
                , mMessage(message)
                , mFile(file)
                , mLine(line)
                , mFunction(function)
                , mTag(tag)
            {}

            void SetAccumulator(int* accumulator)
            {
                mAccumulator = accumulator;
            }

            ~TimeScoper()
            {
                const time::Microsecond_t endTime = platform::GetRealTime(time::kMicrosecondUnit);
                const time::Microsecond_t runTime = endTime - mStartTime;
                const int timeDiff = time::FromTo<int>(runTime, time::kMicrosecondUnit, TU);
                if (mAccumulator)
                {
                    *mAccumulator += timeDiff;
                }

                YLOG_PNOTICE(mTag, mFile, mLine, mFunction, "%s: '%s' (%s).", mMessage, conv::ToThousandsSep(timeDiff).c_str(), UnitName(TU).c_str());
            }

        private:
            time::Microsecond_t mStartTime = 0;
            const char* mMessage = nullptr;
            const char* mFile = "";
            uint32_t mLine = 0;
            const char* mFunction = "";
            const char* mTag = "PROF";
            int* mAccumulator = nullptr;
        };

#if YAGET_METRIC_GATHER == 1

        // used to mark up code and gather metrics over that time. This includes total agragated timing, number of hits and if it has a parent
        // TODO: add support for min, max, avg/mean
        #define YM_GATHER(name) yaget::metric::Scoper YAGET_UNIQUE_NAME(name)(yaget::metric::internal::gatherer, YAGET_STRINGIZE(name), nullptr, false)
        #define YM_GATHER_PARENT(name) yaget::metric::Scoper YAGET_UNIQUE_NAME(name)(yaget::metric::internal::gatherer, YAGET_STRINGIZE(name), nullptr, true)

        #define YM_GATHER_RESET yaget::metric::internal::gatherer.Reset()
        #define YM_GATHER_ACTIVATE(active) yaget::metric::Gather::Activate(active)
#else

        //template<int64_t TU = time::kMicrosecondUnit>
        //class TimeScoper
        //{
        //public:
        //    TimeScoper(const char* message) { YLOG_UNUSED1(message); }
        //};

        #define YM_GATHER(name)             do { YLOG_UNUSED1(YAGET_STRINGIZE(name)); } while(0)
        #define YM_GATHER_PARENT(name)      do { YLOG_UNUSED1(YAGET_STRINGIZE(name)); } while(0)

        #define YM_GATHER_RESET
        #define YM_GATHER_ACTIVATE(active)  do { YLOG_UNUSED1(active); } while(0)

#endif // YAGET_METRIC_GATHER == 1


    } // namespace metric
} // namespace yaget
