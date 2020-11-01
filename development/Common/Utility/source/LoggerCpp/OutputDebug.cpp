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

yaget::ylog::OutputDebug::OutputDebug(const Config::Ptr& aConfigPtr)
    : m_bSplitLines(conv::Convertor<bool>::FromString(aConfigPtr->get("split_lines", "false")))
{}

// Output the Log to the Visual Studio debugger using OutputDebugString()
void yaget::ylog::OutputDebug::OnOutput(const Channel::Ptr& /*aChannelPtr*/, const ylog::Log& aLog) const
{
    const auto& buffer = aLog.FormatedMessage(m_bSplitLines);
    OutputDebugStringA(buffer);
}
#endif  // _WIN32
