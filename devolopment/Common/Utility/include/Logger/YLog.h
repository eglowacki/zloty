///////////////////////////////////////////////////////////////////////
// YLog.h
//
//  Copyright 11/13/2005 Edgar Glowacki.
//
//  Maintained by: Edgar
//
//  NOTES:
//      This is defined in yaget.Options vc++ property sheet
//      #define YAGET_LOG_ENABLED 1
//      Initialize log system, should be called as one of first thing during
//      startup phase. It is called by system::Initialize(...) automaticly
//
//  #include "Logger/YLog.h"
//
//////////////////////////////////////////////////////////////////////
//! \file

#pragma once

#include "LoggerCpp/LoggerCpp.h"
#include "LoggerCpp/Manager.h"
#include "App/Args.h"


namespace yaget
{
    class Logger;
}

namespace yaget
{
    namespace ylog
    {
        // Call this as a first thing in your app to initialize logging sub-system
        void Initialize(const args::Options& options);

        // Return one global instance of Logger object
        Logger& Get();

        struct Tagger
        {
            Tagger(const char* tag) : Tag(*(uint32_t *)tag)
            {
                for (int i = 0; i < 4; ++i)
                {
                    if (tag[i])
                    {
                        mCharTag[i] = tag[i];
                    }
                    else
                    {
                        mCharTag[i] = ' ';
                    }
                }
                mCharTag[4] = '\0';
            }

            operator uint32_t() const { return Tag; }

            union
            {
                uint32_t Tag;
                char mCharTag[5] = { '\0' };
            };
        };

    } // namespace ylog 
} // namespace yaget

#if !defined(YAGET_LOG_ENABLED)
    #if !defined(NDEBUG) // if we are in debug mode
        #define YAGET_LOG_ENABLED 1 // enable it by default, otherwise we honor user settings
    #endif
#endif

#define LOG_TAG(x) yaget::ylog::Tagger(x)

#define YAGET_LOG_FILE_LINE_FUNCTION __FILE__, __LINE__, __FUNCTION__

#if YAGET_LOG_ENABLED == 1

    #define YLOG_DEBUG(tag, ...)                            yaget::ylog::Get().debug().Write(__FILE__, __LINE__, __FUNCTION__, LOG_TAG(tag), true, __VA_ARGS__)
    #define YLOG_INFO(tag, ...)                             yaget::ylog::Get().info().Write(__FILE__, __LINE__, __FUNCTION__, LOG_TAG(tag), true, __VA_ARGS__)
    #define YLOG_NOTICE(tag, ...)                           yaget::ylog::Get().notice().Write(__FILE__, __LINE__, __FUNCTION__, LOG_TAG(tag), true, __VA_ARGS__)
    #define YLOG_WARNING(tag, ...)                          yaget::ylog::Get().warning().Write(__FILE__, __LINE__, __FUNCTION__, LOG_TAG(tag), true, __VA_ARGS__)
    #define YLOG_ERROR(tag, ...)                            yaget::ylog::Get().error().Write(__FILE__, __LINE__, __FUNCTION__, LOG_TAG(tag), true, __VA_ARGS__)
    #define YLOG_CRITICAL(tag, ...)                         yaget::ylog::Get().critic().Write(__FILE__, __LINE__, __FUNCTION__, LOG_TAG(tag), true, __VA_ARGS__)

    #define YLOG_PDEBUG(tag, file, line, function, ...)     yaget::ylog::Get().debug().Write(file, line, function, LOG_TAG(tag), true, __VA_ARGS__)
    #define YLOG_PINFO(tag, file, line, function, ...)      yaget::ylog::Get().info().Write(file, line, function, LOG_TAG(tag), true, __VA_ARGS__)
    #define YLOG_PNOTICE(tag, file, line, function, ...)    yaget::ylog::Get().notice().Write(file, line, function, LOG_TAG(tag), true, __VA_ARGS__)
    #define YLOG_PWARNING(tag, file, line, function, ...)   yaget::ylog::Get().warning().Write(file, line, function, LOG_TAG(tag), true, __VA_ARGS__)
    #define YLOG_PERROR(tag, file, line, function, ...)     yaget::ylog::Get().error().Write(file, line, function, LOG_TAG(tag), true, __VA_ARGS__)

    #define YLOG_CDEBUG(tag, bValid, ...)                   yaget::ylog::Get().debug().Write(__FILE__, __LINE__, __FUNCTION__, LOG_TAG(tag), bValid == false, __VA_ARGS__)
    #define YLOG_CINFO(tag, bValid, ...)                    yaget::ylog::Get().info().Write(__FILE__, __LINE__, __FUNCTION__, LOG_TAG(tag), bValid == false, __VA_ARGS__)
    #define YLOG_CNOTICE(tag, bValid, ...)                  yaget::ylog::Get().notice().Write(__FILE__, __LINE__, __FUNCTION__, LOG_TAG(tag), bValid == false, __VA_ARGS__)
    #define YLOG_CWARNING(tag, bValid, ...)                 yaget::ylog::Get().warning().Write(__FILE__, __LINE__, __FUNCTION__, LOG_TAG(tag), bValid == false, __VA_ARGS__)
    #define YLOG_CERROR(tag, bValid, ...)                   yaget::ylog::Get().error().Write(__FILE__, __LINE__, __FUNCTION__, LOG_TAG(tag), bValid == false, __VA_ARGS__)

    #define YLOG_IS_TAG_VISIBLE(tag)                        (!(yaget::ylog::Manager::IsFilter(LOG_TAG(tag)) || yaget::ylog::Manager::IsOverrideFilter(LOG_TAG(tag))))

#else

    #define YLOG_DEBUG(tag, ...)                            do { YLOG_UNUSED1(tag); YLOG_ALL_UNUSED_IMPL( YLOG_VA_NUM_ARGS(__VA_ARGS__))(__VA_ARGS__ ); } while(0)
    #define YLOG_INFO(tag, ...)                             do { YLOG_UNUSED1(tag); YLOG_ALL_UNUSED_IMPL( YLOG_VA_NUM_ARGS(__VA_ARGS__))(__VA_ARGS__ ); } while(0)
    #define YLOG_NOTICE(tag, ...)                           do { YLOG_UNUSED1(tag); YLOG_ALL_UNUSED_IMPL( YLOG_VA_NUM_ARGS(__VA_ARGS__))(__VA_ARGS__ ); } while(0)
    #define YLOG_WARNING(tag, ...)                          do { YLOG_UNUSED1(tag); YLOG_ALL_UNUSED_IMPL( YLOG_VA_NUM_ARGS(__VA_ARGS__))(__VA_ARGS__ ); } while(0)
    #define YLOG_ERROR(tag, ...)                            do { YLOG_UNUSED1(tag); YLOG_ALL_UNUSED_IMPL( YLOG_VA_NUM_ARGS(__VA_ARGS__))(__VA_ARGS__ ); } while(0)
    #define YLOG_CRITICAL(tag, ...)                         do { YLOG_UNUSED1(tag); YLOG_ALL_UNUSED_IMPL( YLOG_VA_NUM_ARGS(__VA_ARGS__))(__VA_ARGS__ ); } while(0)

    #define YLOG_PDEBUG(tag, file, line, function, ...)     do { YLOG_UNUSED1(tag); YLOG_ALL_UNUSED_IMPL( YLOG_VA_NUM_ARGS(__VA_ARGS__))(__VA_ARGS__ ); } while(0)
    #define YLOG_PINFO(tag, file, line, function, ...)      do { YLOG_UNUSED1(tag); YLOG_ALL_UNUSED_IMPL( YLOG_VA_NUM_ARGS(__VA_ARGS__))(__VA_ARGS__ ); } while(0)
    #define YLOG_PNOTICE(tag, file, line, function, ...)    do { YLOG_UNUSED1(tag); YLOG_ALL_UNUSED_IMPL( YLOG_VA_NUM_ARGS(__VA_ARGS__))(__VA_ARGS__ ); } while(0)
    #define YLOG_PWARNING(tag, file, line, function, ...)   do { YLOG_UNUSED1(tag); YLOG_ALL_UNUSED_IMPL( YLOG_VA_NUM_ARGS(__VA_ARGS__))(__VA_ARGS__ ); } while(0)
    #define YLOG_PERROR(tag, file, line, function, ...)     do { YLOG_UNUSED1(tag); YLOG_ALL_UNUSED_IMPL( YLOG_VA_NUM_ARGS(__VA_ARGS__))(__VA_ARGS__ ); } while(0)

    #define YLOG_CDEBUG(tag, bValid, ...)                   do { YLOG_UNUSED2(tag, bValid); YLOG_ALL_UNUSED_IMPL( YLOG_VA_NUM_ARGS(__VA_ARGS__))(__VA_ARGS__ ); } while(0)
    #define YLOG_CINFO(tag, bValid, ...)                    do { YLOG_UNUSED2(tag, bValid); YLOG_ALL_UNUSED_IMPL( YLOG_VA_NUM_ARGS(__VA_ARGS__))(__VA_ARGS__ ); } while(0)
    #define YLOG_CNOTICE(tag, bValid, ...)                  do { YLOG_UNUSED2(tag, bValid); YLOG_ALL_UNUSED_IMPL( YLOG_VA_NUM_ARGS(__VA_ARGS__))(__VA_ARGS__ ); } while(0)
    #define YLOG_CWARNING(tag, bValid, ...)                 do { YLOG_UNUSED2(tag, bValid); YLOG_ALL_UNUSED_IMPL( YLOG_VA_NUM_ARGS(__VA_ARGS__))(__VA_ARGS__ ); } while(0)
    #define YLOG_CERROR(tag, bValid, ...)                   do { YLOG_UNUSED2(tag, bValid); YLOG_ALL_UNUSED_IMPL( YLOG_VA_NUM_ARGS(__VA_ARGS__))(__VA_ARGS__ ); } while(0)

    #define YLOG_IS_TAG_VISIBLE(tag)                        (false)

#endif // YAGET_LOG_ENABLED == x
