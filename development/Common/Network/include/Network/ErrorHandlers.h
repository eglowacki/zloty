///////////////////////////////////////////////////////////////////////
// ErrorHandlers.h
//
//  Copyright 02/05/2024 Edgar Glowacki.
//
//  Maintained by: Edgar
//
//  NOTES:
//      Utilities related handling errors
//      May need to include this file
//          #include <boost/system/error_code.hpp>
//
//
//  #include "Network/ErrorHandlers.h"
//
//////////////////////////////////////////////////////////////////////
//! \file
#pragma once

#include "Core/ErrorHandlers.h"


namespace boost::system
{
    class error_code;
}

namespace yaget::error_handlers
{
    void ThrowOnError(const boost::system::error_code& ec, const std::string& message, const std::source_location& location = std::source_location::current());

} // namespace yaget::error_handlers
