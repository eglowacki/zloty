///////////////////////////////////////////////////////////////////////
// ErrorHandlers.h
//
//  Copyright 2/23/2024 Edgar Glowacki.

//  Maintained by: Edgar
//
//  NOTES:
//
//  #include "Core/ErrorHandlers.h"
//
//////////////////////////////////////////////////////////////////////
//! \file
#pragma once

#include "YagetCore.h"
#include <source_location>

namespace yaget::error_handlers
{
    void Throw(const char* tag, const std::string& message, const std::source_location& location = std::source_location::current());
    inline void Throw(const std::string& message, const std::source_location& location = std::source_location::current()) { Throw("UTIL", message, location); }
    //void ThrowOnError(bool resultValid, const std::string& message, const std::source_location& location = std::source_location::current());
    void ThrowOnError(long hr, const std::string& message, const std::source_location& location = std::source_location::current());

} // namespace yaget::error_handlers


//#define YAGET_UTIL_THROW_ON_RROR(resultValid, message) yaget::util::ThrowOnError(resultValid, message, __FILE__, __LINE__, __FUNCTION__)
