/**
 * @file    OutputDebug.cpp
 * @ingroup LoggerCpp
 * @brief   Output to the Visual Studio debugger using OutputDebugString()
 *
 * Copyright (c) 2013 Sebastien Rombauts (sebastien.rombauts@gmail.com)
 *
 * Distributed under the MIT License (MIT) (See accompanying file LICENSE.txt
 * or copy at http://opensource.org/licenses/MIT)
 */

#ifdef _WIN32

#include "LoggerCpp/OutputDebug.h"
#include "StringHelpers.h"
#include <cstdio>
#include "Platform/WindowsLean.h"

using namespace yaget;
using namespace yaget::ylog;

OutputDebug::OutputDebug(const Config::Ptr& aConfigPtr)
    : m_bSplitLines(conv::Convertor<bool>::FromString(aConfigPtr->get("split_log", "false")))
{}

// Output the Log to the Visual Studio debugger using OutputDebugString()
void OutputDebug::OnOutput(const Channel::Ptr& aChannelPtr, const ylog::Log& aLog) const
{
    const DateTime& time = aLog.getTime();
    char buffer[1024 * 16] = { '\0' };
    char tag[5] = { '\0' };
    constexpr std::size_t BufferSize = sizeof(buffer);

    if (aLog.GetTag())
    {
        *(reinterpret_cast<uint32_t*>(tag)) = aLog.GetTag();
    }

    int result = 0;
    if (m_bSplitLines)
    {
        result = _snprintf_s(buffer, BufferSize, _TRUNCATE, "%s  %-12s [%s%s%s] %s\n%s(%d) : %s\n", time.ToString().c_str(), aChannelPtr->getName().c_str(),
            Log::toString(aLog.getSeverity()), tag ? ":" : "", tag,
            aLog.getStream().str().c_str(), aLog.GetFileName().c_str(), aLog.GetFileLine(), aLog.GetFunctionName().c_str());
    }
    else
    {
        result = _snprintf_s(buffer, BufferSize, _TRUNCATE, "%s  %-12s [%s%s%s] %s(%d) %s : %s\n", time.ToString().c_str(), aChannelPtr->getName().c_str(),
            Log::toString(aLog.getSeverity()), tag ? ":" : "", tag,
            aLog.GetFileName().c_str(), aLog.GetFileLine(), aLog.getStream().str().c_str(), aLog.GetFunctionName().c_str());

    }

    if (result == -1)
    {
        buffer[BufferSize - 5] = '.';
        buffer[BufferSize - 4] = '.';
        buffer[BufferSize - 3] = '.';
        buffer[BufferSize - 2] = '\n';
        buffer[BufferSize - 1] = '\0';
    }
    
    OutputDebugStringA(buffer);
}
#endif  // _WIN32
