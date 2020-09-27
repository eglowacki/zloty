/////////////////////////////////////////////////////////////////////////
// Exception.h
//
//  Copyright 3/29/2009 Edgar Glowacki.
//
// NOTES:
//      Exception handling classes used in Yaget engine
//
//
// #include "Exception/Exception.h"
//
/////////////////////////////////////////////////////////////////////////
//! \file
#pragma once

#include <exception>
#include <string>


namespace yaget::ex
{
    //! Base exception class which all yaget exception should derive from
    //! Generally, we only try to use exception in creation of major complex object
    //! where it presents challenge to unwind from deep error, and exceptions are well suited for it
    //! NOTE: Add support for preserving line and file from original place of exception
    class standard : public std::exception
    {
    public:
        standard(const std::string& msg) : std::exception(msg.c_str())
        {}
    };

    //! Preferred usage in ctor when initialization fails. In most cases this will abort
    //! program. Pool Allocators handle that internal and return nullptr if component
    //! through that exception
    class bad_init : public standard
    {
    public:
        bad_init(const std::string& msg) : standard(msg)
        {}
    };

} // namespace yaget::ex
