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
        {
            //YLOG_INFO("EXCE", msg.c_str());
        }
    };

    ////! Any serialization error which might leave object in
    ////! unfinished state
    //class serialize : public standard
    //{
    //public:
    //    serialize(const char *msg) : standard(msg)
    //    {}
    //};

    ////! Any runtime script error can generate this exception,
    ////! like compiling and execution
    //class script_runtime : public standard
    //{
    //public:
    //    script_runtime(const char *msg) : standard(msg)
    //    {}
    //};

    //! Any generic error in managing resources/assets
    class bad_resource : public standard
    {
    public:
        bad_resource(const std::string& msg) : standard(msg)
        {}
    };

    ////! Any plugin error in, mostly during loading
    //class plugin : public standard
    //{
    //public:
    //    plugin(const char *msg) : standard(msg)
    //    {}
    //};


    ////! During development we can throw this error for missing functionality
    //class program : public standard
    //{
    //public:
    //    program(const char *msg) : standard(msg)
    //    {}
    //};

    //! Preferred usage in ctor when initialization fails
    class bad_init : public standard
    {
    public:
        bad_init(const std::string& msg) : standard(msg)
        {}
    };

    //! Preferred usage in threads when task fails
    class bad_task : public standard
    {
    public:
        bad_task(const std::string& msg) : standard(msg)
        {}
    };
    ////! Any method/function can throw this exception in ctor for invalid parameters
    //class bad_param : public standard
    //{
    //public:
    //    bad_param(const char *msg) : standard(msg)
    //    {}
    //};

} // namespace yaget::ex
