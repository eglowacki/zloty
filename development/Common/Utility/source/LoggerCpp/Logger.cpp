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


// Initialize a Logger utility object
yaget::ylog::Logger::Logger(const char* apChannelName) : mChannelPtr(Manager::get(apChannelName))
{
    assert(mChannelPtr);
}

// Utility const method to produce Log objets, used to collect the stream to output
yaget::ylog::Log yaget::ylog::Logger::debug() const
{
    return Log(*this, Log::Level::eDebug);
}

yaget::ylog::Log yaget::ylog::Logger::info() const
{
    return Log(*this, Log::Level::eInfo);
}

yaget::ylog::Log yaget::ylog::Logger::notice() const
{
    return Log(*this, Log::Level::eNotice);
}

yaget::ylog::Log yaget::ylog::Logger::warning() const
{
    return Log(*this, Log::Level::eWarning);
}

yaget::ylog::Log yaget::ylog::Logger::error() const
{
    return Log(*this, Log::Level::eError);
}

yaget::ylog::Log yaget::ylog::Logger::critic() const
{
    return Log(*this, Log::Level::eCritic);
}

// To be used only by the Log class
void yaget::ylog::Logger::output(const Log& aLog) const
{
    Manager::output(mChannelPtr, aLog);
}
