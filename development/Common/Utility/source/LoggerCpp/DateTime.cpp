/**
 * @file    DateTime.cpp
 * @ingroup LoggerCpp
 * @brief   Current time precise to the millisecond.
 *
 * Copyright (c) 2013 Sebastien Rombauts (sebastien.rombauts@gmail.com)
 *
 * Distributed under the MIT License (MIT) (See accompanying file LICENSE.txt
 * or copy at http://opensource.org/licenses/MIT)
 */

#include "LoggerCpp/DateTime.h"
#include "LoggerCpp/Utils.h"
#include <cstdio>

#ifdef _WIN32
    #include "Platform/WindowsLean.h"
#else
    #include <time.h>
    #include <sys/time.h>
#endif // _WIN32

using namespace yaget;
using namespace yaget::ylog;

/// Constructor
DateTime::DateTime(void) :
    year(0)
    , month(0)
    , day(0)
    , hour(0)
    , minute(0)
    , second(0)
    , ms(0)
    , us(0)
{ }

/// Set to current time
void DateTime::Make(void)
{
#ifdef _WIN32
    SYSTEMTIME now;
    GetLocalTime(&now);

    year = now.wYear;
    month = now.wMonth;
    day = now.wDay;
    hour = now.wHour;
    minute = now.wMinute;
    second = now.wSecond;
    ms = now.wMilliseconds;
    us = 0;
#else // _WIN32
            struct timeval now;
            gettimeofday(&now, nullptr);
            struct tm* timeinfo = localtime(&now.tv_sec);

            year = timeinfo->tm_year + 1900;
            month = timeinfo->tm_mon + 1;
            day = timeinfo->tm_mday;
            hour = timeinfo->tm_hour;
            minute = timeinfo->tm_min;
            second = timeinfo->tm_sec;
            ms = now.tv_usec / 1000;
            us = now.tv_usec % 1000;
#endif // _WIN32
}

std::string DateTime::ToString() const
{
    char buffer[256];
    sprintf_s(buffer, "%.2u-%.2u-%.4u %.2u:%.2u:%.2u.%.3u", month, day, year, hour, minute, second, ms);

    return std::string(buffer);
}
