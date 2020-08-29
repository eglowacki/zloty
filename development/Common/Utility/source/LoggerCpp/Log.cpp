/**
 * @file    Log.cpp
 * @ingroup LoggerCpp
 * @brief   A RAII (private) log object constructed by the Logger class
 *
 * Copyright (c) 2013 Sebastien Rombauts (sebastien.rombauts@gmail.com)
 *
 * Distributed under the MIT License (MIT) (See accompanying file LICENSE.txt
 * or copy at http://opensource.org/licenses/MIT)
 */

#include "LoggerCpp/Log.h"
#include "LoggerCpp/Logger.h"
#include "LoggerCpp/Manager.h"
#include <stdarg.h> 
#include <cstring>
#include <cassert>
using namespace yaget;
using namespace yaget::ylog;

namespace
{
    const char* vtextprintf(const char* format, va_list vlist)
    {
        int bytes_needed = vsnprintf(nullptr, 0, format, vlist);
        char* buff = new char[bytes_needed + 1];
        vsnprintf(buff, bytes_needed + 1, format, vlist);
        return buff;
    }
} // namespace

// Construct a RAII (private) log object for the Logger class
Log::Log(const Logger& aLogger, Level aSeverity)
    : mLogger(aLogger)
    , mSeverity(aSeverity)
{
    //// Construct a stream only if the severity of the Log is above its Logger Log::Level
    ////if (aSeverity >= aLogger.getLevel())
    //{
    //    mpStream = std::make_shared<std::ostringstream>();
    //}
}

// Destructor : output the Log string stream
Log::~Log()
{
    if (!mIsFiltered)
    {
        mTime.Make();
        FormatLineMessage();
        mLogger.output(*this);
    }
}

// Convert a Level to its string representation
const char* Log::toString(Log::Level aLevel)
{
    const char* pString = nullptr;

    switch (aLevel)
    {
        case Log::Level::eDebug: pString = "DBUG";
            break;
        case Log::Level::eInfo: pString = "INFO";
            break;
        case Log::Level::eNotice: pString = "NOTE";
            break;
        case Log::Level::eWarning: pString = "WARN";
            break;
        case Log::Level::eError: pString = "EROR";
            break;
        case Log::Level::eCritic: pString = "CRIT";
            break;
        default: pString = "????";
            break;
    }

    return pString;
}

// Convert a string representation of a Level to its corresponding value
Log::Level Log::toLevel(const char* apLevel)
{
    Log::Level level;

    if (0 == strncmp(apLevel, "DBUG", 4))
        level = Log::Level::eDebug;
    else if (0 == strncmp(apLevel, "INFO", 4))
        level = Log::Level::eInfo;
    else if (0 == strncmp(apLevel, "NOTE", 4))
        level = Log::Level::eNotice;
    else if (0 == strncmp(apLevel, "WARN", 4))
        level = Log::Level::eWarning;
    else if (0 == strncmp(apLevel, "EROR", 4))
        level = Log::Level::eError;
    else /* (0 == strncmp(apLevel, "CRIT", 4)*/
        level = Log::Level::eCritic; // NOLINT(whitespace/newline)

    return level;
}

void Log::Write(const char* file, unsigned line, const char* functionName, uint32_t tag, bool bValid, const char* format, ...)
{
    // Did you forget to registered this tag?
    // To register your tag in core library add 'YOUR_TAG_NAME"
    // to Logger/LogTags.h file. For external library, executable,
    // add to you GetRegisteredTags() function
    assert(Manager::IsValidTag(tag));

    if (bValid)
    {
        mTag = tag;
        mIsFiltered = true;
        if (Manager::IsOverrideFilter(mTag))
        {
            mIsFiltered = false;
        }
        else if (mSeverity >= mLogger.getLevel())
        {
            mIsFiltered = Manager::IsSeverityFilter(mSeverity, mTag) ? Manager::IsFilter(mTag) : false;
        }

        if (!mIsFiltered)
        {
            mFileName = file ? file : "unknown file";
            mFileLine = line;
            mFunctionName = functionName ? functionName : "unknown function";

            va_list vlist;
            va_start(vlist, format);
            const char* buff = vtextprintf(format, vlist);
            mpStream << buff;
            va_end(vlist);
            delete buff;
        }
    }
    else
    {
        mIsFiltered = true;
    }
}

void Log::FormatLineMessage()
{
    const DateTime& time = getTime();
    const auto& timeText = time.ToString();
    const auto& channelName = mLogger.getName();
    const auto& severityText = Log::toString(getSeverity());
    const auto& streamText = getStream().str();

    char tag[5] = { '\0' };
    *(reinterpret_cast<uint32_t*>(tag)) = GetTag();
    constexpr std::size_t BufferSize = 1024 * 16;

    mFormatedLineMessageSplit.resize(BufferSize);
    char* buffer = mFormatedLineMessageSplit.data();

    int result = _snprintf_s(buffer, BufferSize, _TRUNCATE, "%s  %-12s [%s%s%s] %s\n%s(%d) : %s\n", 
        timeText.c_str(), channelName.c_str(),
        severityText, tag[0] ? ":" : "", tag,
        streamText.c_str(), GetFileName().c_str(), GetFileLine(), GetFunctionName().c_str());

    if (result == -1)
    {
        buffer[BufferSize - 5] = '.';
        buffer[BufferSize - 4] = '.';
        buffer[BufferSize - 3] = '.';
        buffer[BufferSize - 2] = '\n';
        buffer[BufferSize - 1] = '\0';
    }

    mFormatedLineMessage.resize(BufferSize);
    buffer = mFormatedLineMessage.data();

    result = _snprintf_s(buffer, BufferSize, _TRUNCATE, "%s  %-12s [%s%s%s] %s ** %s(%d) : %s\n",
        timeText.c_str(), channelName.c_str(),
        severityText, tag[0] ? ":" : "", tag,
        streamText.c_str(), GetFileName().c_str(), GetFileLine(), GetFunctionName().c_str());

    if (result == -1)
    {
        buffer[BufferSize - 5] = '.';
        buffer[BufferSize - 4] = '.';
        buffer[BufferSize - 3] = '.';
        buffer[BufferSize - 2] = '\n';
        buffer[BufferSize - 1] = '\0';
    }
}
