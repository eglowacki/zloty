/**
 * @file    Logger.cpp
 * @ingroup LoggerCpp
 * @brief   A simple thread-safe Logger class
 *
 * Copyright (c) 2013 Sebastien Rombauts (sebastien.rombauts@gmail.com)
 *
 * Distributed under the MIT License (MIT) (See accompanying file LICENSE.txt
 * or copy at http://opensource.org/licenses/MIT)
 */

#include "LoggerCpp/Logger.h"
#include "LoggerCpp/Manager.h"

#include <cassert>

using namespace yaget;
using namespace yaget::ylog;

// Initialize a Logger utility object
Logger::Logger(const char* apChannelName) : mChannelPtr(Manager::get(apChannelName))
{
    assert(mChannelPtr);
}

// Utility const method to produce Log objets, used to collect the stream to output
Log Logger::debug() const
{
    return Log(*this, Log::Level::eDebug);
}

Log Logger::info() const
{
    return Log(*this, Log::Level::eInfo);
}

Log Logger::notice() const
{
    return Log(*this, Log::Level::eNotice);
}

Log Logger::warning() const
{
    return Log(*this, Log::Level::eWarning);
}

Log Logger::error() const
{
    return Log(*this, Log::Level::eError);
}

Log Logger::critic() const
{
    return Log(*this, Log::Level::eCritic);
}

// To be used only by the Log class
void Logger::output(const Log& aLog) const
{
    Manager::output(mChannelPtr, aLog);
}
