//////////////////////////////////////////////////////////////////////
// Runtime.h
//
//  Copyright 4/18/2020 Edgar Glowacki
//
//  Maintained by: Edgar
//
//  NOTES:
//      missing_notes
//
//
//  #include "Scripting/Runtime.h"
//
//////////////////////////////////////////////////////////////////////
//! \file
#pragma once

#include "YagetCore.h"


namespace yaget::args { class Options; }

namespace yaget::scripting
{
    class Context
    {
    public:
        Context(const char* niceName) : mNiceName(niceName ? niceName : "")
        {}

        virtual ~Context() = default;

    private:
        std::string mNiceName;
    };

    using ContextHandle = std::unique_ptr<Context>;

    ContextHandle CreateContext(const char* niceName);

} // namespace yaget::scripting
